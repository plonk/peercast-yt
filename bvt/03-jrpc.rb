require_relative 'common'
require 'json'

spawn_peercast

["127.0.0.1",
 #"[::1]"
].each do |host|
  response = http_get("http://#{host}:7144/api/1")
  assert_eq(200, response.code)
  json = JSON.parse(response.body)
  assert(json["agentName"] =~ /^PeerCast\/0.1218/)
  assert_eq("1.0.0", json['apiVersion'])
  assert_eq("2.0", json['jsonrpc'])

  response = http_post("http://#{host}:7144/api/1", body: '{"jsonrpc": "2.0","method": "getStatus","id": 9999}')
  json = JSON.parse(response.body)
  assert_eq(9999, json['id'])
  assert_eq('2.0', json['jsonrpc'])
  assert(json['result'].has_key?("globalDirectEndPoint"))
  assert(json['result'].has_key?("globalRelayEndPoint"))
  assert(json['result'].has_key?("isFirewalled"))
  assert(json['result'].has_key?("localDirectEndPoint"))
  assert(json['result'].has_key?("localRelayEndPoint"))
  assert(json['result'].has_key?("uptime"))
end

puts 'ok'
