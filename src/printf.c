#include "types.h"
#include "uart.h"
#include "aarch64.h"
#include "vcpu.h"
#include "lib.h"

#define va_list __builtin_va_list
#define va_start(v, l)  __builtin_va_start(v, l)
#define va_arg(v, l)  __builtin_va_arg(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_copy(d, s) __builtin_va_copy(d, s)

enum printopt {
  PRINT_0X      = 1 << 0,
  ZERO_PADDING  = 1 << 1,
};

static void printiu64(i64 num, int base, bool sign, int digit, enum printopt opt) {
  char buf[sizeof(num) * 8 + 1] = {0};
  char *end = buf + sizeof(buf);
  char *cur = end - 1;
  u64 unum;
  bool neg = false;

  if(sign && num < 0) {
    unum = (u64)(-(num + 1)) + 1;
    neg = true;
  } else {
    unum = (u64)num;
  }

  do {
    *--cur = "0123456789abcdef"[unum % base];
  } while(unum /= base);

  if(opt & PRINT_0X) {
    *--cur = 'x';
    *--cur = '0';
  }

  if(neg)
    *--cur = '-';

  int len = strlen(cur);
  if(digit > 0) {
    while(digit-- > len)
      uart_putc(' '); 
  }
  uart_puts(cur);
  if(digit < 0) {
    digit = -digit;
    while(digit-- > len)
      uart_putc(' '); 
  }
}

static bool isdigit(char c) {
  return '0' <= c && c <= '9';
}

static const char *fetch_digit(const char *fmt, int *digit) {
  int n = 0, neg = 0;

  if(*fmt == '-') {
    fmt++;
    neg = 1;
  }

  while(isdigit(*fmt)) {
    n = n * 10 + *fmt++ - '0';
  }

  *digit = neg? -n : n;

  return fmt;
}

static void printmacaddr(u8 *mac) {
  for(int i = 0; i < 6; i++) {
    printiu64(mac[i], 16, false, 2, 0);
    if(i != 5)
      uart_putc(':');
  }
}

static int vprintf(const char *fmt, va_list ap) {
  char *s;
  void *p;
  int digit = 0;

  for(; *fmt; fmt++) {
    char c = *fmt;
    if(c == '%') {
      fmt++;
      fmt = fetch_digit(fmt, &digit);

      switch(c = *fmt) {
        case 'd':
          printiu64(va_arg(ap, i32), 10, true, digit, 0);
          break;
        case 'u':
          printiu64(va_arg(ap, u32), 10, false, digit, 0);
          break;
        case 'x':
          printiu64(va_arg(ap, u64), 16, false, digit, 0);
          break;
        case 'p':
          p = va_arg(ap, void *);
          printiu64((u64)p, 16, false, digit, PRINT_0X);
          break;
        case 'c':
          uart_putc(va_arg(ap, int));
          break;
        case 's':
          s = va_arg(ap, char *);
          if(!s)
            s = "(null)";

          uart_puts(s);
          break;
        case 'm': /* print mac address */
          printmacaddr(va_arg(ap, u8 *));
          break;
        case '%':
          uart_putc('%');
          break;
        default:
          uart_putc('%');
          uart_putc(c);
          break;
      }
    } else {
      uart_putc(c);
    }
  }

  return 0;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vprintf(fmt, ap);

  va_end(ap);

  return 0;
}

void panic(const char *fmt, ...) {
  intr_disable();

  va_list ap;
  va_start(ap, fmt);

  printf("!!!!!!vmm panic: ");
  vprintf(fmt, ap);
  printf("\n");

  vcpu_dump(cur_vcpu());

  va_end(ap);

  for(;;)
    asm volatile("wfi");
}
