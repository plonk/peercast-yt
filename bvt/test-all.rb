#!/usr/bin/env ruby -W0
Dir.glob("[0-9]*-*.rb").sort.each do |rb|
  name = rb.sub(/\.rb$/, '')
  print "#{name}: "
  STDOUT.flush
  system("ruby", rb)
  unless $?.exitstatus == 0
    puts "failed"
    exit 1
  end
end
