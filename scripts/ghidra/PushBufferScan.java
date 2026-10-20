// Looks for D3D push buffer objects and raw NV2A command writes.
//@category PGR2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import java.util.*;

public class PushBufferScan extends GhidraScript {
    String blk(ghidra.program.model.address.Address a) {
        MemoryBlock b = currentProgram.getMemory().getBlock(a);
        return b == null ? "?" : b.getName();
    }
    String fn(ghidra.program.model.address.Address a) {
        Function f = getFunctionContaining(a);
        return f == null ? "nofunc" : f.getName() + "@" + f.getEntryPoint();
    }
    @Override
    public void run() throws Exception {
        for (MemoryBlock b : currentProgram.getMemory().getBlocks())
            println("block " + b.getName() + " " + b.getStart() + "-" + b.getEnd() + (b.isExecute() ? " x" : ""));
        // D3DCOMMON type field is bits 16-18: 2 = push buffer, 6 = fixup
        long[] tags = {0x20001, 0x60001, 0x00020000, 0x00060000};
        Map<String, Integer> methodCounts = new TreeMap<>();
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction in = it.next();
            String b = blk(in.getAddress());
            for (int op = 0; op < in.getNumOperands(); op++) {
                for (Object o : in.getOpObjects(op)) {
                    if (!(o instanceof Scalar)) continue;
                    long v = ((Scalar) o).getUnsignedValue() & 0xFFFFFFFFL;
                    for (long t : tags)
                        if (v == t) println(String.format("tag %06X at %s [%s] in %s : %s", v, in.getAddress(), b, fn(in.getAddress()), in));
                    // and/test with 0x70000 (type mask)
                    if (v == 0x20000000L || v == 0x1FFFFFFFL || v == 0x1FFFFFFCL || v == 0xE0000003L || v == 0x00000002L && in.getMnemonicString().equals("OR"))
                        println("jumpcall at " + in.getAddress() + " in " + fn(in.getAddress()) + " : " + in);
                    if (v == 0x70000) println("typemask at " + in.getAddress() + " [" + b + "] in " + fn(in.getAddress()) + " : " + in);
                    // NV2A method header: count in 18..28, method 0..0x1FFC, low 2 bits and bits 29,31 clear
                    long n = (v >> 18) & 0x7FF, m = v & 0x1FFC;
                    if ((v & 0xA0030003L) == 0 && n >= 1 && n <= 256 && m >= 0x100 && in.getMnemonicString().startsWith("MOV")
                            && in.getNumOperands() == 2 && op == 1 && in.getOperandType(0) != ghidra.program.model.lang.OperandType.REGISTER) {
                        println(String.format("mstore %s %04X %d %s : %s", in.getAddress(), m, n, fn(in.getAddress()), in));
                    }
                }
            }
        }
        for (Map.Entry<String, Integer> e : methodCounts.entrySet()) println("method-store " + e.getKey() + " x" + e.getValue());
    }
}
