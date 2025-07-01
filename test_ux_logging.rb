#!/usr/bin/env ruby

# Test script to demonstrate UX debugging logging
# Set environment variable to enable logging: set CLIPTIC_UX_DEBUG=1

puts "Cliptic UX Debug Logging Test"
puts "=============================="
puts ""

# Set the environment variable to enable logging
ENV['CLIPTIC_UX_DEBUG'] = '1'

puts "Setting CLIPTIC_UX_DEBUG=1 to enable UX logging..."
puts ""

puts "To enable UX logging for cliptic, run any of these commands:"
puts ""
puts "Windows PowerShell:"
puts "  $env:CLIPTIC_UX_DEBUG=1; ruby bin/cliptic"
puts ""
puts "Windows Command Prompt:"
puts "  set CLIPTIC_UX_DEBUG=1 && ruby bin/cliptic"
puts ""
puts "Linux/macOS:"
puts "  CLIPTIC_UX_DEBUG=1 ruby bin/cliptic"
puts ""

# Check if cliptic is available
if File.exist?("lib/cliptic.rb")
  puts "Testing logger initialization..."
  
  begin
    require_relative "lib/cliptic.rb"
    
    puts "Logger module loaded successfully!"
    puts ""
    
    # Initialize default config to set up the logger
    Cliptic::Config::Default.set
    
    puts "Example log entries that will be created:"
    puts "- Every key press with key codes and context"
    puts "- Menu navigation and selection changes"
    puts "- Screen redraws and window operations"
    puts "- Cursor movements on the game board"
    puts "- Character insertion and deletion"
    puts "- Game state changes (modes, completion, etc.)"
    puts "- Error conditions and recovery attempts"
    puts ""
    
    log_dir = Cliptic::Logger::LOG_DIR
    log_file = Cliptic::Logger::LOG_FILE
    
    puts "Log file location:"
    puts "  Directory: #{log_dir}"
    puts "  File: #{log_file}"
    puts ""
    
    if Dir.exist?(log_dir)
      puts "✓ Log directory exists"
    else
      puts "ℹ Log directory will be created when logging starts"
    end
    
    if File.exist?(log_file)
      size = File.size(log_file)
      puts "✓ Log file exists (#{size} bytes)"
      puts ""
      puts "Recent log entries (last 10 lines):"
      puts "-" * 40
      
      begin
        lines = File.readlines(log_file)
        lines.last(10).each { |line| puts line }
      rescue => e
        puts "Could not read log file: #{e.message}"
      end
    else
      puts "ℹ Log file will be created when logging starts"
    end
    
  rescue => e
    puts "Error testing logger: #{e.message}"
    puts e.backtrace.join("\n") if $DEBUG
  end
else
  puts "cliptic.rb not found. Please run this script from the cliptic root directory."
end

puts ""
puts "To view logs in real-time while playing, use:"
puts "Windows: Get-Content '#{Cliptic::Logger::LOG_FILE}' -Wait"
puts "Linux/macOS: tail -f '#{Cliptic::Logger::LOG_FILE}'"
