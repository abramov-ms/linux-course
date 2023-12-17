#ifndef MMANEG_H
#define MMANEG_H

enum mmaneg_opcode {
    mmaneg_listvma,
    mmaneg_findpage,
    mmaneg_writeval,
};

struct mmaneg_request {
    enum mmaneg_opcode op;
    unsigned long addr;
    unsigned char val;
};

#endif // MMANEG_H
