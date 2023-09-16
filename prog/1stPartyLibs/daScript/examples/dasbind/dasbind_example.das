require dasbind

[extern(stdcall,name="GetCurrentProcessorNumber",library="Kernel32")]
def GetCurrentProcessorNumber : int { return -1; }

[extern(stdcall,name="ExitProcess",library="Kernel32")]
def ExitProcess ( code : int ) {}

[extern(stdcall,name="SetConsoleTitleA",library="Kernel32")]
def SetConsoleTitle(lpConsoleTitle:string) {}

[export]
def main
    let proc_n = GetCurrentProcessorNumber()
    print("current processor {proc_n}\n")
    SetConsoleTitle("hello, console title\n")
    ExitProcess(1234)



