#!/usr/bin/env ruby
require 'cgi'

def main
  cgi = CGI.new

  unless cgi['id'] =~ /\A[a-fA-F0-9]{32}\z/
    puts "Status: 400 Bad Request"
    puts ""
    exit
  end

  id = cgi['id']
  server_name = ENV['SERVER_NAME']
  server_port = ENV['SERVER_PORT']

  print "Content-Type: video/x-flv\n\n"
  system("ffmpeg",
         "-v", "-8", # quiet
         "-y",       # confirm overwriting
         "-i", "mmsh://#{server_name}:#{server_port}/stream/#{id}.wmv",
         "-acodec", "aac",
         "-vcodec", "h264",
         "-preset", "ultrafast",
         "-f", "flv",
         "-")        # to stdout
end

main
