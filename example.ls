func fib(n number) number {
    if (n == 0 || n == 1) {
        return n
    }

    return fib(n - 1) + fib(n - 2)
}

n := fib(30);

n
log
