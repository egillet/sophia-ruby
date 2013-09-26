require 'rubygems'
require 'bundler/setup'
Bundler.require
require 'minitest/spec'
require 'minitest/autorun'
require 'sophia'
require 'tmpdir'

describe Sophia do
  it 'get/set' do
    path = Dir.mktmpdir
    sophia = Sophia.new path
    sophia['key'] = 'value'
    sophia['key'].must_equal 'value'
  end

  it 'fetch' do
    path = Dir.mktmpdir
    sophia = Sophia.new path

    sophia.fetch('key', '123').must_equal '123'
  end

  it 'fetch block' do
    path = Dir.mktmpdir
    sophia = Sophia.new path

    sophia.fetch('key') { |key|
      key.must_equal 'key'
      '456'
    }.must_equal '456'
  end
  
  it 'each' do
    h = {'key 0' => 'value 0', 'key 1' => 'value 1', 'key 2' => 'value 2', '3rd key' => 'value 3'}
    path = Dir.mktmpdir
    sophia = Sophia.new path
    h.each do |k, v|
      sophia[k] = v
    end
    result = []
    sophia.each(){|i| result << i}.must_equal sophia
    result.must_equal h.sort
  end
  
  it 'each kv' do
    h = {'key 0' => 'value 0', 'key 1' => 'value 1', 'key 2' => 'value 2', '3rd key' => 'value 3'}
    path = Dir.mktmpdir
    sophia = Sophia.new path
    h.each do |k, v|
      sophia[k] = v
    end
    result = []
    sophia.each(){|k,v| result << [k, v]}.must_equal sophia
    result.must_equal h.sort
  end
  
  it 'to_a' do
    h = {'key 0' => 'value 0', 'key 1' => 'value 1', 'key 2' => 'value 2', '3rd key' => 'value 3'}
    path = Dir.mktmpdir
    sophia = Sophia.new path
    h.each do |k, v|
      sophia[k] = v
    end
    sophia.to_a.must_equal h.sort
  end
  
  it 'to_h' do
    h = {'key 0' => 'value 0', 'key 1' => 'value 1', 'key 2' => 'value 2', '3rd key' => 'value 3'}
    path = Dir.mktmpdir
    sophia = Sophia.new path
    h.each do |k, v|
      sophia[k] = v
    end
    sophia.to_h.must_equal h
  end

end
