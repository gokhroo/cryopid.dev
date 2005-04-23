#ifndef _CPIMAGE_H_
#define _CPIMAGE_H_

#include <sys/socket.h>
#include <linux/types.h>
#include <linux/in.h>
#include <linux/un.h>
#include <linux/user.h>
#include <linux/unistd.h>
#include <asm/ldt.h>
#include <asm/termios.h>
#include <signal.h>

#include "list.h"

#define IMAGE_VERSION 0x02

#define TRAMPOLINE_ADDR	0x0010000

#define RESUMER_START 0x00100000 /* Lowest location resume will be at */
#define RESUMER_END   0x00200000 /* Highest location resume will be at */

struct k_sigaction {
    __sighandler_t sa_hand;
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    struct {
	unsigned long sig[2];
    } sa_mask;       /* mask last for extensibility */
};

static inline int set_rt_sigaction(int sig, const struct k_sigaction* ksa,
	const struct k_sigaction* oksa) {
    int ret;
    asm (
	    "int $0x80"
	    : "=a"(ret)
	    : "a"(__NR_rt_sigaction), "b"(sig),
	      "c"(ksa), "d"(oksa), "S"(sizeof(ksa->sa_mask))
	);
    return ret;
}

#define GET_LIBRARIES_TOO          0x01
#define GET_OPEN_FILE_CONTENTS     0x02

/* Constants for cp_chunk.type */
#define CP_CHUNK_MISC		0x01
#define CP_CHUNK_REGS		0x02
#define CP_CHUNK_I387_DATA	0x03
#define CP_CHUNK_TLS		0x04
#define CP_CHUNK_FD		0x05
#define CP_CHUNK_VMA		0x06
#define CP_CHUNK_SIGHAND	0x07
#define CP_CHUNK_FINAL		0x08

#define CP_CHUNK_MAGIC		0xC01D

struct cp_misc {
	char *cmdline;
	char *cwd;
	char *env;
};

struct cp_regs {
	struct user *user_data;
};

struct cp_i387_data {
	struct user_i387_struct* i387_data;
};

struct cp_tls {
	struct user_desc* u;
};

struct cp_vma {
    long start, length;
    int prot;
    int flags;
    int dev;
    long pg_off;
    long inode;
    char *filename;
    int have_data;
    void* data; /* length end-start */ /* in file, simply true if is data */
};

struct cp_sighand {
	int sig_num;
	struct k_sigaction *ksa;
};

struct cp_console {
    struct termios termios;
};

struct cp_file {
    char *filename;
    int mode;
    char *contents;
};

struct cp_socket_tcp {
	struct sockaddr_in sin;
	void *ici; /* If the system supports tcpcp. */
};

struct cp_socket_udp {
	struct sockaddr_in sin;
};

struct cp_socket_unix {
	struct sockaddr_un sun;
};

struct cp_socket {
	int proto;
	union {
		struct cp_socket_tcp s_tcp;
		struct cp_socket_udp s_udp;
		struct cp_socket_unix s_unix;
	};
};

struct cp_fd {
	int fd;
	int mode;
	int type;
	int close_on_exec;
	union {
		struct cp_console console;
		struct cp_file file;
		struct cp_socket socket;
	};
};

struct cp_chunk {
	int type;
	union {
		struct cp_misc misc;
		struct cp_regs regs;
		struct cp_i387_data i387_data;
		struct cp_tls tls;
		struct cp_fd fd;
		struct cp_vma vma;
		struct cp_sighand sighand;
	};
};

struct stream_ops {
	void *(*init)(int fd, int mode);
	void (*finish)(void *data);
	int (*read)(void *data, void *buf, int len);
	int (*write)(void *data, void *buf, int len);
} *stream_ops;


/* cpimage.c */
void read_bit(void *fptr, void *buf, int len);
void write_bit(void *fptr, void *buf, int len);
char *read_string(void *fptr, char *buf, int maxlen);
void write_string(void *fptr, char *buf);
int read_chunk(void *fptr, struct cp_chunk **chunkp, int load);
void write_chunk(void *fptr, struct cp_chunk *chunk);
void write_process(int fd, struct list l);
void get_process(pid_t pid, int flags, struct list *l);

/* cp_misc.c */
void read_chunk_misc(void *fptr, struct cp_misc *data, int load);
void write_chunk_misc(void *fptr, struct cp_misc *data);
void process_chunk_misc(struct cp_misc *data);

/* cp_regs.c */
void read_chunk_regs(void *fptr, struct cp_regs *data, int load);
void write_chunk_regs(void *fptr, struct cp_regs *data);
void fetch_chunks_regs(pid_t pid, int flags, struct list *process_image);

/* cp_i387.c */
void read_chunk_i387_data(void *fptr, struct cp_i387_data *data, int load);
void write_chunk_i387_data(void *fptr, struct cp_i387_data *data);
void process_chunk_i387_data(struct cp_i387_data *data);

/* cp_tls.c */
void read_chunk_tls(void *fptr, struct cp_tls *data, int load);
void write_chunk_tls(void *fptr, struct cp_tls *data);
void fetch_chunks_tls(pid_t pid, int flags, struct list *l);
extern int tls_hack;

/* cp_fd.c */
void read_chunk_fd(void *fptr, struct cp_fd *data, int load);
void write_chunk_fd(void *fptr, struct cp_fd *data);
void fetch_chunks_fd(pid_t pid, int flags, struct list *l);

/* cp_vma.c */
void read_chunk_vma(void *fptr, struct cp_vma *data, int load);
void write_chunk_vma(void *fptr, struct cp_vma *data);
void fetch_chunks_vma(pid_t pid, int flags, struct list *l);
extern int extra_prot_flags;
extern long scribble_zone;

/* cp_sighand.c */
void read_chunk_sighand(void *fptr, struct cp_sighand *data, int load);
void write_chunk_sighand(void *fptr, struct cp_sighand *data);
void fetch_chunks_sighand(pid_t pid, int flags, struct list *l);

#endif /* _CPIMAGE_H_ */

/* vim:set ts=8 sw=4 noet: */