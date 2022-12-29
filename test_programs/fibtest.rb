def fib(n)
  if n < 2
    return n
  else
    return fib(n - 2) + fib(n - 1)
  end
end

start = Time.now
puts fib(35)
finish = Time.now
puts (finish - start)
