options gen2

// `anthropic/anthropic` is fetched at runtime by daspkg, not built in.
// If this require fails with "module not found", run from project root:
//   bin/daslang utils/daspkg/main.das -- install --root utils/find-dupe
require anthropic/anthropic
require cluster_input
require judge


def public make_report_verdict_tool() : Tool {
    return <- Tool(
        name = "report_verdict",
        description = "Report your partition of the cluster. Call exactly once.",
        input_schema = VERDICT_SCHEMA
    )
}


// `live_judge` matches the JudgeFn signature so the test harness can
// swap in a fake judge.
//
// We call `create_message` directly (one-shot) rather than
// `run_tool_loop` because the loop crashes on a known das-claude bug:
// it iterates `msg.content` twice, but the first pass moves the
// `_tool_use.input` payload via `deconst` + `<- input`, leaving the
// array in a state that fails the GC magic check on the second
// iteration's unlock. A one-shot call sidesteps the second pass — we
// only need the tool_use args, not the full loop.
def public live_judge(members : array<InputMemberRef>; sources : array<string>; model : string) : Verdict {
    var v : Verdict
    var config <- claude_config_from_env(model)
    var params : MessageCreateParams
    params.system := SYSTEM_PROMPT
    params.messages |> emplace(user_message(format_cluster_prompt(members, sources)))
    params.tools |> emplace(make_report_verdict_tool())
    let n = length(members)
    var error : string
    var msg = create_message(config, params, error)
    // Check error BEFORE reading `msg.usage` — on failure `msg` may be
    // in an indeterminate state, and we'd record bogus token counts.
    if (!empty(error)) {
        v.error := error
        return <- v
    }
    v.input_tokens = msg.usage.input_tokens
    v.output_tokens = msg.usage.output_tokens
    var found = false
    for (blk in msg.content) {
        if (blk is _tool_use) {
            let tc & = unsafe(blk as _tool_use)
            if (tc.name == "report_verdict") {
                parse_verdict(tc.input, n, v)
                found = true
                break
            }
        }
    }
    if (!found && empty(v.error)) {
        v.error := "model did not call report_verdict (returned text only?)"
    }
    return <- v
}
