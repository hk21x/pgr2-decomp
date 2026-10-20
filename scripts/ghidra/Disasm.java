// Disasm.java <hex address> <count>: instructions from there
//@category PGR2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;

public class Disasm extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        for (int k = 0; k + 1 < a.length; k += 2) {
            Instruction in = getInstructionAt(toAddr(Long.parseLong(a[k], 16)));
            if (in == null) in = getInstructionAfter(toAddr(Long.parseLong(a[k], 16)));
            for (int i = 0; i < Integer.parseInt(a[k + 1]) && in != null; i++, in = in.getNext())
                println(in.getAddress() + "  " + in);
        }
    }
}
