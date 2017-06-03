#!/usr/bin/env ruby
require 'json'
require 'cgi'

require_relative 'common'
require_relative 'bbs_reader'

include Bbs

cgi = CGI.new

if cgi["category"].empty? || cgi["board_num"].empty?
  print_bad_request "category and board_num required"
  exit
end

board = Board.new(cgi["category"], cgi["board_num"].to_i)

settings = board.settings
if settings.has_key?("ERROR")
  data = {
    "status" => "error",
    "message"=> settings["ERROR"]
  }
else
  title = board.settings["BBS_TITLE"]
  threads = board.threads.map do |thread|
    {
      "id" => thread.id,
      "title" => thread.title,
      "last" => thread.last
    }
  end

  data = {
    "status" => "ok",
    "title" => title,
    "threads" => threads,
    "category" => board.category,
    "board_num" => board.board_num
  }
end

puts "Content-Type: text/json; charset=UTF-8\n\n"
print JSON.generate(data)
