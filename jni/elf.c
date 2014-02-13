/*
 * elf.c
 *
 *  Created on: Jun 4, 2011
 *      Author: d
 */

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#ifdef ANDROID
//#include <linker.h>
#else
#include <link.h>
#include <elf.h>
#endif








void get_elf_info(int pid, Elf32_Addr base, struct elf_info *einfo) {


    int i = 0;

    einfo->pid = pid;
    einfo->base = base;
    ptrace_read(pid, einfo->base, &einfo->ehdr, sizeof(Elf32_Ehdr));
    einfo->phdr_addr = einfo->base + einfo->ehdr.e_phoff;
    puint(einfo->phdr_addr);
    puint(einfo->ehdr.e_type);

    ptrace_read(pid, einfo->phdr_addr, &einfo->phdr, sizeof(Elf32_Phdr));
//    puint(einfo->phdr.p_align);
//    puint(einfo->phdr.p_filesz);
//    puint(einfo->phdr.p_flags);
//    puint(einfo->phdr.p_memsz);
//    puint(einfo->phdr.p_offset);
//    puint(einfo->phdr.p_paddr);
//    puint(einfo->phdr.p_type);
//    puint(einfo->phdr.p_vaddr);
    LOGD("dump %d phdr\n", einfo->ehdr.e_phnum);
    for(i=0; i < einfo->ehdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        ptrace_read(pid, einfo->phdr_addr + i * sizeof(Elf32_Phdr), &phdr, sizeof(Elf32_Phdr));
//        LOGD(">\n");
//        puint(phdr.p_align);
//        puint(phdr.p_filesz);
//        puint(phdr.p_flags);
//        puint(phdr.p_memsz);
//        puint(phdr.p_offset);
//        puint(phdr.p_paddr);
//        puint(phdr.p_type);
//        puint(phdr.p_vaddr);
    }
    while (einfo->phdr.p_type != PT_DYNAMIC) {
        ptrace_read(pid, einfo->phdr_addr += sizeof(Elf32_Phdr), &einfo->phdr, sizeof(Elf32_Phdr));
    }
    einfo->dynaddr =  (IS_DYN(einfo)?einfo->base:0) + einfo->phdr.p_vaddr;
    pint(einfo->dynaddr);
    ptrace_read(pid, einfo->dynaddr, &einfo->dyn, sizeof(Elf32_Dyn));
    while (einfo->dyn.d_tag != DT_PLTGOT) {
        ptrace_read(pid, einfo->dynaddr + i * sizeof(Elf32_Dyn), &einfo->dyn, sizeof(Elf32_Dyn));
        i++;
    }

    einfo->got = (IS_DYN(einfo)?einfo->base:0) + (Elf32_Word) einfo->dyn.d_un.d_ptr;
    pint(einfo->got);
    ptrace_read(pid, einfo->got+4, &einfo->map_addr, 4);
    pint(einfo->map_addr);

    //ptrace_read(pid, map_addr, map, sizeof(struct link_map));


    //return map;
}

/**
 * 查找符号的重定位地址
 */
unsigned long find_sym_in_rel(struct elf_info *einfo, char *sym_name) {
    Elf32_Rel rel;
    Elf32_Sym sym;
    unsigned int i;
    char *str = NULL;
    unsigned long ret;
    struct dyn_info dinfo;
    LOGD("find sym in rel %p %s \n", (void*)einfo->base, sym_name);

    get_dyn_info(einfo, &dinfo);
    pint(dinfo.nrels);
    pint(dinfo.jmprel);
    pint(dinfo.relsize);
    pint(dinfo.totalrelsize);
    for (i = 0; i < dinfo.nrels; i++) {
        ptrace_read(einfo->pid, (unsigned long) (dinfo.jmprel + i * sizeof(Elf32_Rel)), &rel, sizeof(Elf32_Rel));
        LOGD("rel addr %p\n", &rel);

        if (ELF32_R_SYM(rel.r_info)) {
            ptrace_read(einfo->pid, dinfo.symtab + ELF32_R_SYM(rel.r_info) * sizeof(Elf32_Sym), &sym, sizeof(Elf32_Sym));
            str = ptrace_readstr(einfo->pid, dinfo.strtab + sym.st_name);
            LOGD("   str-> %s\n", str);
            if (strcmp(str, sym_name) == 0) {
                free(str);
                break;
            }
            free(str);
        }
    }

    if (i == dinfo.nrels)
        ret = 0;
    else {
            ret = (IS_DYN(einfo)?einfo->base:0) + rel.r_offset;
    }
    pint(ret);
    return ret;
}



/*
 * 在进程自身的映象中（即不包括动态共享库，无须遍历link_map链表）获得各种动态信息
 */
void get_dyn_info(struct elf_info *einfo, struct dyn_info *dinfo) {
    Elf32_Dyn dyn;
    int i = 0;

    LOGD("get_dyn_info 0x%08x...\n",einfo->dynaddr);

    ptrace_read(einfo->pid, einfo->dynaddr + i * sizeof(Elf32_Dyn), &dyn, sizeof(Elf32_Dyn));
    i++;
    while (dyn.d_tag) {
        switch (dyn.d_tag) {
        case DT_SYMTAB:
            puts("DT_SYMTAB");
            dinfo->symtab = (IS_DYN(einfo)?einfo->base:0) + dyn.d_un.d_ptr;
            break;
        case DT_STRTAB:
            dinfo->strtab = (IS_DYN(einfo)?einfo->base:0) + dyn.d_un.d_ptr;
            puts("DT_STRTAB");
            break;
        case DT_JMPREL:
            dinfo->jmprel = (IS_DYN(einfo)?einfo->base:0) + dyn.d_un.d_ptr;
            puts("DT_JMPREL");
            LOGD("jmprel\t %08x\n", dinfo->jmprel);
            break;
        case DT_PLTRELSZ:
            puts("DT_PLTRELSZ");
            dinfo->totalrelsize = dyn.d_un.d_val;
            LOGD("totalrelsize %d\n", dinfo->totalrelsize);
            break;
        case DT_RELAENT:
            puts("DT_RELAENT");

            dinfo->relsize = dyn.d_un.d_val;
            LOGD("relsize %d\n", dinfo->relsize);
            break;
        case DT_RELENT:
            puts("DT_RELENT");

            dinfo->relsize = dyn.d_un.d_val;
            LOGD("relsize2 %d\n", dinfo->relsize);
            //puts("DT_RELENT");
            break;
        }
        ptrace_read(einfo->pid, einfo->dynaddr + i * sizeof(Elf32_Dyn), &dyn, sizeof(Elf32_Dyn));
        i++;
    }
    if (dinfo->relsize == 0) {
        dinfo->relsize = 8;
    }
    dinfo->nrels = dinfo->totalrelsize / dinfo->relsize;

}




void replace_all_rels(int pid, char *funcname, long addr, char **sos) {
    FILE *m = NULL;
    unsigned int i=0;
    char maps[80];
    char line[200];
    char soaddrs[20];
    char soaddr[10];
    char soname[60];
    char prop[10];
    long soaddval;
    long base;
    memset(maps,0,sizeof(maps));
    memset(soaddrs,0,sizeof(soaddrs));
    memset(soaddr,0,sizeof(soaddr));
    LOGD(maps,"/proc/%d/maps",pid);
    m = fopen(maps,"r");
    if(!m)
    {
        LOGD("open %s error!\n",maps);
    }
    while(fgets(line,sizeof(line),m))
    {
        int in_so_list = 0;
        struct elf_info einfo;
        long tmpaddr = 0;

        if(strstr(line,".so") == NULL && i != 0)continue;
        if(strstr(line,"r-xp") == NULL)continue;

        for(i = 0; sos[i]!=NULL; i++) {
            if(strstr(line,sos[i]) != NULL) {
                in_so_list = 1;
            }
        }
        if(!in_so_list) {
            continue;
        }
        sscanf(line,"%s %s %*s %*s %*s %s",soaddrs,prop,soname);
        sscanf(soaddrs,"%[^-]",soaddr);
        LOGD("#### %s %s %s\n",soaddr,prop,soname);
        base = strtoul(soaddr,NULL, 16);
        puint(base);
        get_elf_info(pid, base, &einfo);
        tmpaddr = find_sym_in_rel(&einfo,funcname);

        puint(tmpaddr);
        if(tmpaddr != 0) {
            ptrace_write(pid,tmpaddr,&addr,4);
            LOGD("base of %-40s     %08x  %08x %08x\n",soname, (int)base, (int)tmpaddr, (int)addr);

        }
    }
}



