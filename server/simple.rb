#!/usr/bin/env ruby2.0
# Server side of muni display. Serves simple number of mins to next train.
# Configuration file maps a url path to a set of nextbus query options. 
# see the full nextbus api spec linked from README.md for available options.
require 'rubygems'
require 'bundler/setup'

require 'faraday'
require 'nokogiri'
require 'optparse'
require 'safe_yaml'

options = {}
OptionParser.new do |opts|
  opts.on("--config FILE", "Config file containing device options") do |f|
    options[:configfile] = File.expand_path(f)
  end
end.parse!


#Note: sinatra uses OptionParser, too. this require needs to be below our option parsing.
require 'sinatra'

SafeYAML::OPTIONS[:default_mode] = :safe
SafeYAML::OPTIONS[:deserialize_symbols] = true

BASEURL =  'http://webservices.nextbus.com'
URL_PATH = '/service/publicXMLFeed'

set :bind, '0.0.0.0'
disable :show_exceptions

#http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=sf-muni&r=N
# inbound duboce and church: 4448

DEFAULTOPTS = {
  command: 'predictions',
  a: 'sf-muni',
  r: 'N',
  s: '4447',
  dir: 'inbound',
}

CONFIG = YAML.load_file(options[:configfile])
p CONFIG

def get_minutes(opts)
  opts = DEFAULTOPTS.merge(opts)
  p opts
  fetch = Faraday.new(BASEURL, {request: {open_timeout: 5}}).get(URL_PATH, opts)
  predictions_in_mins = []
  xmldoc = Nokogiri::XML(fetch.body)
  xmldoc.remove_namespaces!()
  xmldoc.css("predictions direction").each do |node|
    p node['title']
    # verify that our :dir option is in the name of this direction.
    if node['title'].downcase().include?(opts[:dir].downcase())
      node.css("prediction").each do |pnode|
        predictions_in_mins << pnode['minutes'].to_i
      end
    end
    #predictions_in_mins << node['minutes'].to_i
  end
  p predictions_in_mins.sort()
  if predictions_in_mins.length == 0
    # No predictions found. return an unreasonable number.
    p "No predictions found:"
    p fetch.body
    return [ 99 ]
  end
  return predictions_in_mins.sort()
end

get '/times/:device' do |dev|
  options = {}
  if CONFIG.has_key?(dev)
    options.merge!(CONFIG[dev])
  end
  return get_minutes(options)[0].to_s
end
