require 'sinatra'

get '/times/:device' do |dev|
  puts "checkin from #{dev}"
  "8"
end
