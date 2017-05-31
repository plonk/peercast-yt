
def hp(obj)
  print CGI.escapeHTML obj.inspect
end

def print_bad_request(message)
  puts "Status: 400 Bad Request"
  puts "Content-Type: text/plain"
  puts
  puts message
end
