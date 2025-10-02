# frozen_string_literal: true

require "bundler/gem_tasks"
require "rake/extensiontask"

Rake::ExtensionTask.new "quadmath" do |ext|
  ext.lib_dir = "lib/quadmath"
end

require "minitest/test_task"

Minitest::TestTask.create

task default: :test
