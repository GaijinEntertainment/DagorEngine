options gen2
options rtti

module tool_result

require daslib/json_boost public

//! The MCP `tools/call` result envelope every server in this tree answers with: one text
//! content item, flagged when it is an error.

struct ContentItem {
    @rename = "type" _type : string
    text : string
}

struct ToolResult {
    content : array<ContentItem>
    isError : bool
}

def make_tool_result(text : string; is_error : bool = false) : string {
    var result : ToolResult
    result.isError = is_error
    result.content |> emplace(ContentItem(_type = "text", text = text))
    return sprint_json(result, false)
}
