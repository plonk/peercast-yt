#!/usr/bin/env ruby
require 'json'
require 'cgi'

require_relative 'common'
require_relative 'bbs_reader'

include Bbs

cgi = CGI.new

if cgi["url"].empty?
  print_bad_request "url required"
  exit
end

if shitaraba_board? cgi["url"]
  board = board_from_url(cgi['url'])
elsif shitaraba_thread? cgi['url']
  thread = thread_from_url cgi['url']
  board = thread.board
else
  print_bad_request "bad url"
  exit
end

settings = board.settings
if settings.has_key?("ERROR")
  title = "板設定取得エラー"
else
  title = board.settings["BBS_TITLE"]
end

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

puts "Content-Type: text/json; charset=UTF-8\n\n"
print JSON.generate(data, indent: "  ", object_nl: "\n", array_nl: "\n", space: " ")
