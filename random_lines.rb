@counter = 0

def rand32
  str = ''
  32.times do
    str += ['0','1'].sample
  end
  str
end

File.open('trace6.txt','w') do |f|
  50.times do
    base = rand32
    1000.times do
      num = base
      num[-1] = ['0','1'].sample
      num[-2] = ['0','1'].sample
      num[-3] = ['0','1'].sample
      num[-4] = ['0','1'].sample
      num[-5] = ['0','1'].sample
      num[-6] = ['0','1'].sample
      f.puts num.to_i(2)
    end
  end
end
