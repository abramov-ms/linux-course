#define PHONEBOOK_NAME_MAX 64

struct phonebook_user {
    char first_name[PHONEBOOK_NAME_MAX];
    char last_name[PHONEBOOK_NAME_MAX];
    int age;
    long long phone_number;
};

enum phonebook_opcode : char {
    phonebook_get,
    phonebook_create,
    phonebook_remove
};

struct phonebook_request {
    enum phonebook_opcode opcode;
    struct phonebook_user payload;
};
