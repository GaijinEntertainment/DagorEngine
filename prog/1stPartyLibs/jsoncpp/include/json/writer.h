// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef JSON_WRITER_H_INCLUDED
# define JSON_WRITER_H_INCLUDED


#if !defined(JSON_IS_AMALGAMATION)
# include "value.h"
#endif // if !defined(JSON_IS_AMALGAMATION)
# include <EASTL/vector.h>
# include <EASTL/string.h>

namespace Json
{
   class Value;

   /** \brief Abstract class for writers.
    */
   class JSON_API Writer
   {
   public:
      virtual ~Writer();

      virtual eastl::string write( const Value &root ) = 0;
   };

   /** \brief Outputs a Value in <a HREF="http://www.json.org">JSON</a> format without formatting (not human friendly).
    *
    * The JSON document is written in a single line. It is not intended for 'human' consumption,
    * but may be usefull to support feature such as RPC where bandwith is limited.
    * \sa Reader, Value
    */
   class JSON_API FastWriter : public Writer
   {
   public:
      FastWriter();
      virtual ~FastWriter(){}

      void enableYAMLCompatibility();

   public: // overridden from Writer
      virtual eastl::string write( const Value &root );

   private:
      void writeValue( const Value &value );

      eastl::string document_;
      bool yamlCompatiblityEnabled_;
   };

   /** \brief Writes a Value in <a HREF="http://www.json.org">JSON</a> format in a human friendly way.
    *
    * The rules for line break and indent are as follow:
    * - Object value:
    *     - if empty then print {} without indent and line break
    *     - if not empty the print '{', line break & indent, print one value per line
    *       and then unindent and line break and print '}'.
    * - Array value:
    *     - if empty then print [] without indent and line break
    *     - if the array contains no object value, empty array or some other value types,
    *       and all the values fit on one lines, then print the array on a single line.
    *     - otherwise, it the values do not fit on one line, or the array contains
    *       object or non empty array, then print one value per line.
    *
    * If the Value have comments then they are outputed according to their #CommentPlacement.
    *
    * \sa Reader, Value, Value::setComment()
    */
   class JSON_API StyledWriter: public Writer
   {
   public:
      StyledWriter();
      virtual ~StyledWriter(){}

   public: // overridden from Writer
      /** \brief Serialize a Value in <a HREF="http://www.json.org">JSON</a> format.
       * \param root Value to serialize.
       * \return String containing the JSON document that represents the root value.
       */
      virtual eastl::string write( const Value &root );

   private:
      void writeValue( const Value &value );
      void writeArrayValue( const Value &value );
      bool isMultineArray( const Value &value );
      void pushValue( const eastl::string &value );
      void writeIndent();
      void writeWithIndent( const eastl::string &value );
      void indent();
      void unindent();
      void writeCommentBeforeValue( const Value &root );
      void writeCommentAfterValueOnSameLine( const Value &root );
      bool hasCommentForValue( const Value &value );
      static eastl::string normalizeEOL( const eastl::string &text );

      typedef eastl::vector<eastl::string> ChildValues;

      ChildValues childValues_;
      eastl::string document_;
      eastl::string indentString_;
      int rightMargin_;
      int indentSize_;
      bool addChildValues_;
   };

# if defined(JSON_HAS_INT64)
   eastl::string JSON_API valueToString( Int value );
   eastl::string JSON_API valueToString( UInt value );
# endif // if defined(JSON_HAS_INT64)
   eastl::string JSON_API valueToString( LargestInt value );
   eastl::string JSON_API valueToString( LargestUInt value );
   eastl::string JSON_API valueToString( double value );
   eastl::string JSON_API valueToString( bool value );
   eastl::string JSON_API valueToQuotedString( const char *value );

} // namespace Json



#endif // JSON_WRITER_H_INCLUDED
