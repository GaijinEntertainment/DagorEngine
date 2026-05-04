// Tutorial PUGIXML-03: XPath Queries
//
// This tutorial covers:
//   - Selecting text with select_text
//   - Selecting values with select_value (attributes and text)
//   - Iterating XPath results with for_each_select
//   - Compiled XPath queries with with_xpath
//   - Low-level XPath: select_node, select_nodes, each iterator
//
// requires: daslib/PUGIXML_boost (re-exports pugixml)
// Run: daslang.exe tutorials/dasPUGIXML/03_xpath.das

options gen2
options persistent_heap

require pugixml/PUGIXML_boost

let CATALOG_XML = "<catalog>
  <product id=\"A1\" category=\"electronics\">
    <name>Wireless Mouse</name>
    <price>29.99</price>
    <rating>4.5</rating>
  </product>
  <product id=\"A2\" category=\"electronics\">
    <name>Keyboard</name>
    <price>79.99</price>
    <rating>4.8</rating>
  </product>
  <product id=\"B1\" category=\"books\">
    <name>daslang Handbook</name>
    <price>49.99</price>
    <rating>4.9</rating>
  </product>
  <product id=\"B2\" category=\"books\">
    <name>XML in Practice</name>
    <price>34.99</price>
    <rating>4.2</rating>
  </product>
</catalog>"

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — select_text: get text from XPath
// ──────────────────────────────────────────────────────────────────────────
//
// select_text runs an XPath query and returns the text content of
// the first matching node. Returns a default if nothing matches.

def select_text_demo() {
    print("=== select_text ===\n")

    parse_xml(CATALOG_XML) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // Get text of the first product's name
        let first_name = select_text(root, "product[1]/name")
        print("first product: {first_name}\n")
        // output: first product: Wireless Mouse

        // Get by attribute filter
        let book_name = select_text(root, "product[@category='books']/name")
        print("first book: {book_name}\n")
        // output: first book: daslang Handbook

        // Missing path returns default
        let missing = select_text(root, "product/description", "N/A")
        print("description: {missing}\n")
        // output: description: N/A
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — select_value: get attribute or text values
// ──────────────────────────────────────────────────────────────────────────
//
// select_value returns the value of the first matching XPath result.
// If the result is an attribute, returns its value. If an element,
// returns its text content.

def select_value_demo() {
    print("\n=== select_value ===\n")

    parse_xml(CATALOG_XML) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // Attribute value via XPath
        let id = select_value(root, "product[1]/@id")
        print("first id: {id}\n")
        // output: first id: A1

        let cat = select_value(root, "product[@id='B1']/@category")
        print("B1 category: {cat}\n")
        // output: B1 category: books

        // Element text via XPath
        let price = select_value(root, "product[@id='A2']/price")
        print("A2 price: {price}\n")
        // output: A2 price: 79.99

        // Default when missing
        let missing = select_value(root, "product/@color", "none")
        print("color: {missing}\n")
        // output: color: none
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — for_each_select: iterate XPath results
// ──────────────────────────────────────────────────────────────────────────
//
// for_each_select runs an XPath query and invokes a block for each
// matching node. The xpath_node_set is freed automatically.

def for_each_select_demo() {
    print("\n=== for_each_select ===\n")

    parse_xml(CATALOG_XML) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // All product names
        print("All products:\n")
        root |> for_each_select("product/name") <| $(xn) {
            let n = xn.node
            print("  {n.text as string}\n")
        }
        // output: All products:
        // output:   Wireless Mouse
        // output:   Keyboard
        // output:   daslang Handbook
        // output:   XML in Practice

        // Filter: only electronics
        print("Electronics:\n")
        root |> for_each_select("product[@category='electronics']/name") <| $(xn) {
            let n = xn.node
            print("  {n.text as string}\n")
        }
        // output: Electronics:
        // output:   Wireless Mouse
        // output:   Keyboard

        // Select attributes
        print("All IDs:\n")
        root |> for_each_select("product/@id") <| $(xn) {
            let a = xn.attribute
            print("  {a as string}\n")
        }
        // output: All IDs:
        // output:   A1
        // output:   A2
        // output:   B1
        // output:   B2
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Compiled XPath with with_xpath
// ──────────────────────────────────────────────────────────────────────────
//
// with_xpath compiles an XPath expression once and frees it after
// the block. Useful when you need to evaluate the same query on
// multiple nodes or want to check for compilation errors.

def compiled_xpath_demo() {
    print("\n=== Compiled XPath ===\n")

    parse_xml(CATALOG_XML) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        with_xpath("product/name") <| $(query) {
            // Evaluate on the root
            var ns = evaluate_node_set(query, root)
            print("Product count: {ns.size}\n")
            // output: Product count: 4

            // Iterate the result set
            ns |> for_each() <| $(xn) {
                let n = xn.node
                print("  {n.text as string}\n")
            }
            unsafe { delete ns; }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Low-level XPath: select_node, select_nodes, each
// ──────────────────────────────────────────────────────────────────────────
//
// For more control, use select_node (first match) and select_nodes
// (all matches) directly on an xml_node.  The `each` iterator makes
// xpath_node_set? directly usable in `for` loops.

def low_level_xpath() {
    print("\n=== Low-level XPath ===\n")

    parse_xml(CATALOG_XML) <| $(doc, ok) {
        if (!ok) { return ; }
        let root = doc.document_element

        // select_node — first matching node
        let result = select_node(root, "product[price > 40]/name")
        if (result.ok) {
            let n = result.node
            print("first expensive: {n.text as string}\n")
        }
        // output: first expensive: Keyboard

        // select_nodes — iterate directly with a for loop
        var ns = select_nodes(root, "product[price < 40]/@id")
        print("cheap products: {ns.size}\n")
        for (xn in ns) {
            let a = xn.attribute
            print("  {a as string}\n")
        }
        // output: cheap products: 2
        // output:   A1
        // output:   B2
        unsafe { delete ns; }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Practical example: querying books.xml
// ──────────────────────────────────────────────────────────────────────────

def querying_books() {
    print("\n=== Querying books.xml ===\n")

    open_xml("tutorials/dasPUGIXML/books.xml") <| $(doc, ok) {
        if (!ok) {
            print("could not open books.xml\n")
            return
        }
        let root = doc.document_element

        // Count books by language using XPath with select_nodes
        var en_books = select_nodes(root, "book[@lang='en']")
        print("English books: {en_books.size}\n")
        // output: English books: 3
        unsafe { delete en_books; }

        // Find cheapest book
        let cheapest = select_text(root, "book[not(price > ../book/price)]/title")
        print("cheapest: {cheapest}\n")

        // All authors (may contain duplicates)
        print("All authors:\n")
        root |> for_each_select("book/author") <| $(xn) {
            let n = xn.node
            print("  {n.text as string}\n")
        }
    }
}

[export]
def main() {
    select_text_demo()
    select_value_demo()
    for_each_select_demo()
    compiled_xpath_demo()
    low_level_xpath()
    querying_books()
}
