#!/usr/bin/env ruby
require 'json'
require 'cgi'

require_relative 'common'
require_relative 'bbs_reader'

include Bbs

cgi = CGI.new

if cgi["category"].empty? || cgi["board_num"].empty? || cgi["id"].empty?
  print_bad_request "bad parameter"
  exit
end

board = Board.new(cgi["category"], cgi["board_num"].to_i)
thread = board.thread(cgi["id"].to_i)

if thread.nil?
  puts "Content-Type: text/json; charset=UTF-8\n\n"
  print({
    "status" => "error",
    "message" => "そんなスレッドないです。"
  }.to_json)
  exit
end

posts = thread.posts(1..Float::INFINITY).map do |post|
  {
    "no" => post.no,
    "name" => post.name,
    "mail" => post.mail,
    "body" => post.body,
    "date" => post.date_str
  }
end

puts "Content-Type: text/json; charset=UTF-8\n\n"
print({
  "status" => "ok",
  "posts" => posts
}.to_json)
