
#include "text.h"
#include <cctype>

#define self (*this)

/* TODO: Hanlde hex escape sequences */

namespace text_internal
{


	string &quote(string &s)
    {
		if (s[0] != '"' || s.back() != '"') {
        	s.insert(s.begin(), '"');
            s += '"';
		}
        return s;
    }    

	string &unquote(string &s)
    {
    	if (s[0] == '"' && s.back() == '"') {
        	s.erase(0, 1);
            s.erase(s.size() - 1);
		}
        return s;
    }

	string &escape(string &s)
    {
    	for (string::iterator i = s.begin(); i != s.end(); ++i)
        	if (*i == '\n') s.replace(i, i + 1, "\\n"); else
				if (*i == '\r') s.replace(i, i + 1, "\\r"); else
					if (*i == '\\') s.replace(i, i + 1, "\\\\");
		return s;
    }

  	string& unescape(string &s)
  	{
  		for (string::size_type i = 0; i < s.size(); ++i)
  			if (s[i] == '\\' && (s.erase(i, 1), i < s.size()) ) {

				char c = s[i];
  				switch(c) {
				case 'n': c = '\n'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				case 'b': c = '\b'; break;	
				case 'r': c = '\r'; break;
				case 'f': c = '\f'; break;
				case 'a': c = '\a'; break;
				case '?': c = '\?'; break;
				case '\'': c = '\''; break;
				case '"': c = '"'; break;
				case '\\': c = '\\'; break;
  				}
				s[i] = c;
			}
		return s;
  	}

	void
    columnizer::add_column(unsigned width, const string &text)
	{
       	static string sep(" \t\n\r");
       	widthes.push_back(width);
		list<string> words;
		split(text, sep, words);
		texts.push_back(words);
	}

    void
    columnizer::flush(ostream &os) {
       	while(1) {
           	string line;
           	string tmp;
			bool flag = false;
            for (unsigned i = 0; i < texts.size(); ++i) { 
				tmp.clear();
              	while( texts[i].size() &&
					   tmp.size() + 1 + texts[i].front().size() <= widthes[i])
				{
					(tmp += " ") += texts[i].front();
					texts[i].pop_front();
				}
                if (tmp.empty() && !texts[i].empty()) {
               		(tmp = " ") += texts[i].front().substr(0, widthes[i] - 1);
                   	texts[i].front().erase(0, widthes[i] - 1);
                }
				line += tmp;
                flag |= tmp.size();
                line.append(widthes[i] - tmp.size(), ' ');
			}
            if (!flag) break;
			os << line << endl;
       	}
       	widthes.clear();
        texts.clear();
	}

    string format(const char *fs, ... )
    {
    	va_list ap;
        va_start(ap, fs);
		int needed = std::vsnprintf(NULL, 0, fs, ap);
        char *buf = new char[needed + 1];
        std::vsnprintf(buf, needed + 1, fs, ap);
        va_end(ap);
        string result(buf);
        delete [] buf;
        return result;
    }

    #ifndef NO_PCRE
	smart_regexp::smart_regexp(const char *regexp, int flags)
    	: _matches(NULL), current_match(1),
		  compiled_regexp(new cr_ptr(init(regexp, flags)))
    {
    }

    smart_regexp::smart_regexp(const char *regexp, const string &s, int flags)
    	: _matches(NULL), current_match(1),
		  compiled_regexp(new cr_ptr(init(regexp, flags)))
	{
        (*this)(s);
    }
	
	pcre* smart_regexp::init(const char *regexp, int flags)
    {
  		const char *error_message;
	    int  error_pos;
		pcre *res = pcre_compile(regexp, flags, &error_message, &error_pos, NULL);   	
	    if (!res)
    		throw logic_error(format("regexp error at %d: %s", error_pos, error_message));
		return res;
	}

	bool smart_regexp::operator()(const string &s)
	{
    	rewind();
    	last_try = s;

        if (_matches == NULL) {
			int nsp = pcre_info(*compiled_regexp, NULL, NULL) + 1;
    		matches_count = ((nsp + 1) / 2)*3;
	   		_matches = new pair<int, int>[matches_count];
		}            	

		int r = pcre_exec(*compiled_regexp, NULL, s.c_str(), s.size(),
						  0, reinterpret_cast<int*>(_matches), matches_count*2);

		if (r >= 0) return true;
        if (r == PCRE_ERROR_NOMATCH) return false;
	    throw logic_error("error while matching");        	
	}

    string smart_regexp::operator[](size_t i)
    {
    	if (_matches[i].first == -1) return string();
    	return
			last_try.substr(_matches[i].first, _matches[i].second - _matches[i].first);
    }

	string smart_regexp::replace(const string &_replacement)
	{
		string replacement(_replacement);
		unescape(replacement);
  		string::size_type pos = matches()[0].first, end = matches()[0].second;

		string r(last_try); r.replace(pos, end - pos, replacement);
        end = pos + replacement.length();

		while( /* backslash found */ pos = r.find('\\', pos), pos < end - 1) {
						
			string::size_type pos2 = r.find_first_not_of("0123456789", pos + 1);
			if (pos2 == pos + 1) 
				++pos;
			else {
				if (pos2 == -1) pos2 = r.size();
				int val = atoi(r.substr(pos + 1, pos2 - pos - 1));
				if (val >= matches_count && matches()[val].first == -1)
					throw logic_error("Invalid subpattern number");
				r.replace(pos, pos2 - pos, self[val]);
				int s = self[val].length() - (pos2 - pos);
				end += s;
				pos = pos2;
			}
		}
		return r;
	}

	template<class T>
    smart_regexp& smart_regexp::operator>>(T &t)
    {
    	stringstream aux(self[current_match++]);
        aux >> t;
        return self;
    }

	template<>
	smart_regexp& smart_regexp::operator>>(string &s)
    {
    	s = self[current_match++];
        return self;
    }

	void smart_regexp::rewind() { current_match = 1; }

	smart_regexp::~smart_regexp()
    {	
    	delete[] _matches;
    	//	not needed thanks to ref counting
    	//  pcre_free(compiled_regexp);
	}
    #endif

}

#ifdef SELF_TEST
#include <iostream>
int main()
{
	using namespace text;
    using namespace std;

    try {
	    cout << escape(quote(format("%d %f\n", 10, 10.1))) << endl;

	    columnizer col;
    	col.add_column(6, "Desciption <cr> of nothing");
	    col.add_column(15, "Hi!!  Hellow!!!!!!!!! La-la");
    	col.add_column(30, "This is an example of a program the can format text in a cool fashion without any efforts on your behalf");
		col.flush(cout);

        formatter::test();

		smart_regexp r("( \\w* \\ * & )?" "\\ *(\\w+)" "\\ *:\\ *" "(.$)");
        r(" m : m");
        string s1, s2, s3;
        r >> s1 >> s2 >> s3;
        cout << endl << s1 << endl << s2 << endl << s3 << endl;
        
	}
    catch(exception &e)
    {
		cout << e.what() << endl;
    }


}
#endif
