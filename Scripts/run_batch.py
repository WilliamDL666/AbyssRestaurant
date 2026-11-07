# Runs several setup scripts in one editor session, then quits. Usage: -ExecutePythonScript=run_batch.py -AbyssBatch=a.py,b.py -AbyssSetupQuit
import os
import unreal

here = os.path.dirname(os.path.abspath(__file__))
cmd = unreal.SystemLibrary.get_command_line()
names = []
for tok in cmd.split():
    if tok.startswith("-AbyssBatch="):
        names = tok.split("=", 1)[1].split(",")
for n in names:
    p = os.path.join(here, n)
    unreal.log("[AbyssSetup] running %s" % n)
    try:
        exec(compile(open(p, encoding="utf-8").read(), p, "exec"), {"__name__": "__main__", "__file__": p})
    except Exception as e:
        import traceback
        unreal.log_error("[AbyssSetup] %s FAILED: %s\n%s" % (n, e, traceback.format_exc()))
if "-AbyssSetupQuit" in cmd:
    unreal.SystemLibrary.quit_editor()
