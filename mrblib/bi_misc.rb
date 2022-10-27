
alias :_original_rand_ :rand
def rand(n=0)
  if n.is_a? Range
    return n.min if n.min == n.max
    n.min + _original_rand_(1+n.max-n.min)
  else
    _original_rand_(n)
  end
end

def coinflip
  rand(2) == 0
end
