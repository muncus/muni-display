# Statically export an arbitrary number of minutes.
require 'sinatra'

get '/times/:device' do |dev|
  puts "checkin from #{dev}"
  "8"
end
