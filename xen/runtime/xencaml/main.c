/*
 * Copyright (c) 2011 Anil Madhavapeddy <anil@recoil.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <mini-os/os.h>
#include <mini-os/sched.h>
#include <mini-os/hypervisor.h>
#include <mini-os/kernel.h>

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/callback.h>

#include <limits.h>
# define HUGE_VAL	(__builtin_huge_val())

void _exit(int);
int errno;
static char *argv[] = { "mirage", NULL };
static unsigned long irqflags;

CAMLprim value
caml_block_domain(value v_timeout)
{
  CAMLparam1(v_timeout);
  s_time_t block_nsecs = (s_time_t)(Double_val(v_timeout) * 1000000000);
  HYPERVISOR_set_timer_op(NOW() + block_nsecs);
  /* xen/common/schedule.c:do_block clears evtchn_upcall_mask
     to re-enable interrupts. It blocks the domain and immediately
     checks for pending events which otherwise may be missed. */
  HYPERVISOR_sched_op(SCHEDOP_block, 0);
  /* set evtchn_upcall_mask: there's no need to be interrupted
     when we know we have outstanding work to do. When we next
     call this function, the call to SCHEDOP_block will check
     for pending events. */
  local_irq_disable();
  CAMLreturn(Val_unit);
}

#define CAML_ENTRYPOINT "OS.Main.run"

void app_main(start_info_t *si)
{
  value *v_main;
  int caml_completed = 0;
  printk("xencaml: app_main\n");
  local_irq_save(irqflags);
  caml_startup(argv);
  v_main = caml_named_value(CAML_ENTRYPOINT);
  if (v_main == NULL){
	printk("ERROR: CAML_ENTRYPOINT %s is NULL\n", CAML_ENTRYPOINT);
	_exit(1);
  }
  while (caml_completed == 0) {
    caml_completed = Bool_val(caml_callback(*v_main, Val_unit));
  }
  _exit(0);
}

void _exit(int ret)
{
  printk("main returned %d\n", ret);
  stop_kernel();
  if (!ret) {
    /* No problem, just shutdown.  */
    struct sched_shutdown sched_shutdown = { .reason = SHUTDOWN_poweroff };
    HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
  }
  do_exit();
}

#define     ENOSYS          38      /* Function not implemented */

#define print_unsupported(fmt, ...) \
    printk("Unsupported function "fmt" called in Mini-OS kernel\n", ## __VA_ARGS__);

/* Crash on function call */
#define unsupported_function_crash(function) \
    int __unsup_##function(void) asm(#function); \
    int __unsup_##function(void) \
    { \
	print_unsupported(#function); \
	do_exit(); \
    }

/* Log and err out on function call */
#define unsupported_function_log(type, function, ret) \
    type __unsup_##function(void) asm(#function); \
    type __unsup_##function(void) \
    { \
	print_unsupported(#function); \
	errno = ENOSYS; \
	return ret; \
    }

/* Err out on function call */
#define unsupported_function(type, function, ret) \
    type __unsup_##function(void) asm(#function); \
    type __unsup_##function(void) \
    { \
	errno = ENOSYS; \
	return ret; \
    }

char *getenv(const char *name)
{
  printk("getenv(%s) -> null\n", name);
  return NULL;
}

void* calloc(size_t nmemb, size_t _size)
{
  register size_t size=_size*nmemb;
  void* x=malloc(size);
  memset(x,0,size);
  return x;
}

ssize_t write(int fd, const void *buf, size_t count)
{
  if (fd == 1 || fd == 2)
  {
    console_print(opt_console_dev, buf, count);
  }
  else
  {
    printk("Write to FD %d: '%*s'\n", fd, count, buf);
  }
  return count;
}

void exit(int status)
{
  printk("Mirage exiting with status %d\n", status);
  do_exit();
}

/* Not supported by FS yet.  */
unsupported_function_crash(link);
unsupported_function(int, readlink, -1);
unsupported_function_crash(umask);

/* We could support that.  */
unsupported_function_log(int, chdir, -1);

/* No dynamic library support.  */
unsupported_function_log(void *, dlopen, NULL);
unsupported_function_log(void *, dlsym, NULL);
unsupported_function_log(char *, dlerror, NULL);
unsupported_function_log(int, dlclose, -1);

/* We don't raise signals anyway.  */
unsupported_function(int, sigemptyset, -1);
unsupported_function(int, sigfillset, -1);
unsupported_function(int, sigaddset, -1);
unsupported_function(int, sigdelset, -1);
unsupported_function(int, sigismember, -1);
unsupported_function(int, sigprocmask, -1);
unsupported_function(int, sigaction, -1);
unsupported_function(int, __sigsetjmp, 0);
unsupported_function(int, sigaltstack, -1);
unsupported_function_crash(kill);

/* Unsupported */
unsupported_function_crash(pipe);
unsupported_function_crash(fork);
unsupported_function_crash(execv);
unsupported_function_crash(execve);
unsupported_function_crash(waitpid);
unsupported_function_crash(wait);
unsupported_function_crash(lockf);
unsupported_function_crash(sysconf);
unsupported_function(int, tcsetattr, -1);
unsupported_function(int, tcgetattr, 0);
unsupported_function(int, grantpt, -1);
unsupported_function(int, unlockpt, -1);
unsupported_function(char *, ptsname, NULL);

/* net/if.h */
unsupported_function_log(unsigned int, if_nametoindex, -1);
unsupported_function_log(char *, if_indextoname, (char *) NULL);
unsupported_function_log(struct  if_nameindex *, if_nameindex, (struct  if_nameindex *) NULL);
unsupported_function_crash(if_freenameindex);

/* Linuxish abi for the Caml runtime, don't support 
   Log, and return an error code if possible.  If it is not possible
   to inform the application of an error, then crash instead!
*/
unsupported_function_log(struct dirent *, readdir64, NULL);
unsupported_function_log(int, getrusage, -1);
unsupported_function_log(int, getrlimit, -1);
unsupported_function_log(int, getrlimit64, -1);
unsupported_function_log(int, __xstat64, -1);
unsupported_function_log(long, __strtol_internal, LONG_MIN);
unsupported_function_log(double, __strtod_internal, HUGE_VAL);
unsupported_function_log(int, utime, -1);
unsupported_function_log(int, truncate64, -1);
unsupported_function_log(int, tcflow, -1);
unsupported_function_log(int, tcflush, -1);
unsupported_function_log(int, tcdrain, -1);
unsupported_function_log(int, tcsendbreak, -1);
unsupported_function_log(int, cfsetospeed, -1);
unsupported_function_log(int, cfsetispeed, -1);
unsupported_function_crash(cfgetospeed);
unsupported_function_crash(cfgetispeed);
unsupported_function_log(int, symlink, -1);
unsupported_function_log(const char*, inet_ntop, NULL);
unsupported_function_crash(__fxstat64);
unsupported_function_crash(__lxstat64);
unsupported_function_log(int, socketpair, -1);
unsupported_function_crash(sigsuspend);
unsupported_function_log(int, sigpending, -1);
unsupported_function_log(int, shutdown, -1);
unsupported_function_log(int, setuid, -1);
unsupported_function_log(int, setgid, -1);
unsupported_function_crash(rewinddir);
unsupported_function_log(int, getpriority, -1);
unsupported_function_log(int, setpriority, -1);
unsupported_function_log(int, mkfifo, -1);
unsupported_function_log(int, getitimer, -1);
unsupported_function_log(int, setitimer, -1);
unsupported_function_log(void *, getservbyport, NULL);
unsupported_function_log(void *, getservbyname, NULL);
unsupported_function_log(void *, getpwuid, NULL);
unsupported_function_log(void *, getpwnam, NULL);
unsupported_function_log(void *, getprotobynumber, NULL);
unsupported_function_log(void *, getprotobyname, NULL);
unsupported_function_log(int, getpeername, -1);
unsupported_function_log(int, getnameinfo, -1);
unsupported_function_log(char *, getlogin, NULL);
unsupported_function_crash(__h_errno_location);
unsupported_function_log(int, gethostbyname_r, -1);
unsupported_function_log(int, gethostbyaddr_r, -1);
unsupported_function_log(int, getgroups, -1);
unsupported_function_log(void *, getgrgid, NULL);
unsupported_function_log(void *, getgrnam, NULL);
unsupported_function_log(int, getaddrinfo, -1);
unsupported_function_log(int, freeaddrinfo, -1);
unsupported_function_log(int, ftruncate64, -1);
unsupported_function_log(int, fchown, -1);
unsupported_function_log(int, fchmod, -1);
unsupported_function_crash(execvp);
unsupported_function_log(int, dup, -1)
unsupported_function_log(int, chroot, -1)
unsupported_function_log(int, chown, -1);
unsupported_function_log(int, chmod, -1);
unsupported_function_crash(alarm);
unsupported_function_log(int, inet_pton, -1);
unsupported_function_log(int, access, -1);

unsupported_function_crash(open64);
unsupported_function_crash(stat);
unsupported_function_crash(lstat);
unsupported_function_crash(unlink);
unsupported_function_crash(getcwd);
unsupported_function_crash(system);
unsupported_function_crash(close);
unsupported_function_log(off_t, lseek, -1);
unsupported_function_crash(fcntl);
unsupported_function_crash(read);
unsupported_function_crash(gmtime);
unsupported_function_crash(strtod);
unsupported_function_crash(atoi);
unsupported_function_crash(rename);
unsupported_function_crash(times);
unsupported_function_crash(strerror);
