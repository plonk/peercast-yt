require 'typhoeus'
require_relative 'common'

spawn_peercast

response = Typhoeus.get("http://127.0.0.1:7144")
assert_eq(404, response.code)

puts 'ok'
