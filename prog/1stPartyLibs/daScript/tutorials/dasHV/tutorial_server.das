// Shared test server for dasHV tutorials
//
// Provides a self-contained HTTP server on localhost so tutorials
// can make real HTTP requests without external dependencies.
//
// The server registers several endpoints:
//   GET  /ping   — returns "pong"
//   POST /echo   — returns the request body
//   GET  /data   — returns "resource state"
//   GET  /json   — returns a JSON object
//
// Usage:
//   require tutorial_server
//
//   [export]
//   def main() {
//       with_tutorial_server(18080) <| $(base_url) {
//           GET("{base_url}/ping") <| $(resp) {
//               print("{resp.body}\n")
//           }
//       }
//   }
//
// The server runs on a dedicated thread with a tick loop.
// All setup and teardown is handled by with_tutorial_server.

options gen2

require dashv/dashv_boost public
require daslib/jobque_boost
require daslib/fio

// ──────────────────────────────────────────────────────────────────────────
// Server class
// ──────────────────────────────────────────────────────────────────────────

class TutorialServer : HvWebServer {
    def override onInit {
        // GET /ping — simple health-check, returns "pong"
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }
        // POST /echo — returns whatever body was sent
        POST("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN(string(req.body))
        }
        // GET /data — a static resource
        GET("/data") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("resource state")
        }
        // GET /json — returns a JSON object
        GET("/json") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("\{\"status\":\"ok\",\"value\":42\}")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Lifecycle helper
// ──────────────────────────────────────────────────────────────────────────
//
// with_tutorial_server starts the server on a dedicated thread that ticks
// in a loop, runs the user's block, then shuts everything down.
//
// Internally hv::WebSocketServer::start() creates network accept/worker
// threads, and tick() processes the WebSocket event queue + calls onTick.
// We run the tick loop on a separate OS thread via new_thread so the
// caller's block can execute HTTP requests on the main thread without
// blocking.
//
// Synchronization:
//   - JobStatus `started`  : thread signals after server->start()
//   - JobStatus `finished` : thread signals after server->stop()
//   - Atomic32  `stop_flag`: main thread sets to 1 to request shutdown

def with_tutorial_server(port : int; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new TutorialServer()
                    server->init(port)
                    server->start()
                    started |> notify_and_release   // signal: server is listening
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(10u)
                    }
                    server->stop()
                    unsafe {
                        delete server
                    }
                    finished |> notify_and_release  // signal: thread is done
                }
                started |> join                      // wait until server is ready
                let base_url = "http://127.0.0.1:{port}"
                invoke(blk, base_url)
                stop_flag |> set(1)                  // request shutdown
                finished |> join                     // wait until thread exits
            }
        }
    }
}
