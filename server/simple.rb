require 'faraday'
require 'nokogiri'
require 'safe_yaml'
require 'sinatra'

SafeYAML::OPTIONS[:deserialize_symbols] = true

BASEURL =  'http://webservices.nextbus.com/service/publicXMLFeed'

#http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=sf-muni&r=N
# inbound duboce and church: 4448

DEFAULTOPTS = {
  command: 'predictions',
  a: 'sf-muni',
  r: 'N',
  s: '4447'
}

CONFIG = YAML.load_file('config.yaml')
p CONFIG

def get_minutes(opts)
  opts = DEFAULTOPTS.merge(opts)
  p opts
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
  options = {}
  if CONFIG.has_key?(dev)
    options.merge!(CONFIG[dev])
  end
  return get_minutes(options)[0].to_s
end
