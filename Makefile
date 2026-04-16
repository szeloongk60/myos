BUILD = build

OBJS_BOOTPACK = $(BUILD)/bootpack.obj $(BUILD)/naskfunc.obj $(BUILD)/hankaku.obj $(BUILD)/graphic.obj $(BUILD)/dsctbl.obj \
		$(BUILD)/int.obj $(BUILD)/fifo.obj $(BUILD)/keyboard.obj $(BUILD)/mouse.obj $(BUILD)/memory.obj $(BUILD)/sheet.obj $(BUILD)/timer.obj \
		$(BUILD)/mtask.obj $(BUILD)/window.obj $(BUILD)/console.obj $(BUILD)/file.obj $(BUILD)/fd.obj $(BUILD)/lz4.obj
TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# 默认动作

default :
	$(MAKE) img

# 镜像文件生成

$(BUILD)/ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas $(BUILD)/ipl10.bin $(BUILD)/ipl10.lst

$(BUILD)/asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas $(BUILD)/asmhead.bin $(BUILD)/asmhead.lst

$(BUILD)/hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt $(BUILD)/hankaku.bin

$(BUILD)/hankaku.obj : $(BUILD)/hankaku.bin Makefile
	$(BIN2OBJ) $(BUILD)/hankaku.bin $(BUILD)/hankaku.obj _hankaku

$(BUILD)/bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(BUILD)/bootpack.bim stack:3136k map:$(BUILD)/bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

$(BUILD)/bootpack.hrb : $(BUILD)/bootpack.bim Makefile
	$(BIM2HRB) $(BUILD)/bootpack.bim $(BUILD)/bootpack.hrb 0

$(BUILD)/hlt.hrb : hlt.nas Makefile
	$(NASK) hlt.nas $(BUILD)/hlt.hrb $(BUILD)/hlt.lst

$(BUILD)/haribote.sys : $(BUILD)/asmhead.bin $(BUILD)/bootpack.hrb $(BUILD)/hlt.hrb Makefile
	copy /B "$(BUILD)\asmhead.bin"+"$(BUILD)\bootpack.hrb" "$(BUILD)\haribote.sys"
$(BUILD)/naskfunc.nas :
	copy naskfunc.nas build
haribote.img : $(BUILD)/ipl10.bin $(BUILD)/haribote.sys Makefile rule110.bf
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:$(BUILD)/ipl10.bin len:512 from:0 to:0 \
		copy from:$(BUILD)/haribote.sys to:@: \
		copy from:rule110.bf to:@: \
		copy from:cur.lz4 to:@: \
		copy from:1.lz4 to:@: \
		copy from:HZK16.lz4 to:@: \
		copy from:msotxt.lz4 to:@: \
		copy from:imgchat.dat to:@: \
		imgout:haribote.img

# 其他指令

$(BUILD)/%.gas : %.c bootpack.h Makefile
	$(CC1) -o $(BUILD)/$*.gas $*.c

$(BUILD)/%.nas : $(BUILD)/%.gas Makefile
	$(GAS2NASK) $(BUILD)/$*.gas $(BUILD)/$*.nas

$(BUILD)/%.obj : $(BUILD)/%.nas Makefile
	$(NASK) $(BUILD)/$*.nas $(BUILD)/$*.obj $(BUILD)/$*.lst

# 运行程序

img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img
