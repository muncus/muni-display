require 'sinatra'
require 'faraday'
require 'nokogiri'

BASEURL =  'http://webservices.nextbus.com/service/publicXMLFeed'

#http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=sf-muni&r=N
# inbound duboce and church: 4448

DEFAULTOPTS = {
  command: 'predictions',
  a: 'sf-muni',
  r: 'N',
  s: '4448'
}

def get_minutes(opts)
  opts = DEFAULTOPTS.merge(opts)
  fetch = Faraday.get(BASEURL, opts)
  predictions_in_mins = []
  xmldoc = Nokogiri::XML(fetch.body)
  xmldoc.remove_namespaces!()
  xmldoc.css("predictions direction prediction").each do |node|
    predictions_in_mins << node['minutes'].to_i
  end
  p predictions_in_mins.sort()
  return predictions_in_mins.sort()
end

get '/times/:device' do |dev|
  return get_minutes({})[0].to_s
end
