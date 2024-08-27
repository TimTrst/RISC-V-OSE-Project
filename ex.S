.globl ex
.globl exception
.align 4
ex:
        // physadresse des letzten prozesses speichern wir im mscratch register (siehe kernel.c)
        csrrw a0, mscratch, a0

        // problem, der sp zeigt auf eine virtuelle adresse
        // austauschen der beiden register werte a0 in scr mscratch und mscratch in a0
        // in mscratch können temp werte abgespeichert werden
        // so haben wir a0 zur verfügung
        
        // laden der physischen basisadresse des stacks in a0

        // jetzt können wir platz auf dem stack machen 
        addi sp, sp, -256
        add sp, sp, a0 
        //zeigt nun auf startbereich des stacks im virtuellen speicher für den letzten prozess

        // erneutes austauschen von mscratch und a0+
        // dadurch haben wir wieder den originalen wert in register a0
        csrrw a0, mscratch, a0

        // save the registers.
        sd ra, 0(sp)
        sd sp, 8(sp)
        sd gp, 16(sp)
        sd tp, 24(sp)
        sd t0, 32(sp)
        sd t1, 40(sp)
        sd t2, 48(sp)
        sd s0, 56(sp)
        sd s1, 64(sp)
        sd a0, 72(sp)
        sd a1, 80(sp)
        sd a2, 88(sp)
        sd a3, 96(sp)
        sd a4, 104(sp)
        sd a5, 112(sp)
        sd a6, 120(sp)
        sd a7, 128(sp)
        sd s2, 136(sp)
        sd s3, 144(sp)
        sd s4, 152(sp)
        sd s5, 160(sp)
        sd s6, 168(sp)
        sd s7, 176(sp)
        sd s8, 184(sp)
        sd s9, 192(sp)
        sd s10, 200(sp)
        sd s11, 208(sp)
        sd t3, 216(sp)
        sd t4, 224(sp)
        sd t5, 232(sp)
        sd t6, 240(sp)

        mv a0, sp

        // call the C trap handler in kernel.c
        // (kann es sein, dass der wert des sp wieder woanders (kernel?) verändert wird)
        call exception

       // a0 contains the value of SP

        // restore registers.
        ld ra, 0(a0)
        ld sp, 8(a0)
        ld gp, 16(a0)
        ld tp, 24(a0)
        ld t0, 32(a0)
        ld t1, 40(a0)
        ld t2, 48(a0)
        ld s0, 56(a0)
        ld s1, 64(a0)
        ld a1, 80(a0)
        ld a2, 88(a0)
        ld a3, 96(a0)
        ld a4, 104(a0)
        ld a5, 112(a0)
        ld a6, 120(a0)
        ld a7, 128(a0)
        ld s2, 136(a0)
        ld s3, 144(a0)
        ld s4, 152(a0)
        ld s5, 160(a0)
        ld s6, 168(a0)
        ld s7, 176(a0)
        ld s8, 184(a0)
        ld s9, 192(a0)
        ld s10, 200(a0)
        ld s11, 208(a0)
        ld t3, 216(a0)
        ld t4, 224(a0)
        ld t5, 232(a0)
        ld t6, 240(a0)
        ld a0, 72(a0) // bis hier brauchen wir noch a0 (welche den sp enhält)


        addi sp, sp, 256

        // laden der pysbase adresse in a0 und a0 in mscratch (austauschen)
        csrrw a0, mscratch, a0

        // anpassen des sp damit dieser auf pa zeigt (page adresse)
        sub sp, sp, a0

        // wieder den originalen wert nach a0 zurück swappen
        csrrw a0, mscratch, a0

        // return to whatever we were doing in the kernel.
        mret

