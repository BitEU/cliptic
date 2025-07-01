module Cliptic
  module CursesHelper
    # Add a method to check if Curses is closed
    def self.closed?
      !Curses.stdscr || Curses.closed?
    rescue
      true
    end
  end
end

# Monkey patch Curses module to add closed? method if it doesn't exist
module Curses
  def self.closed?
    !stdscr
  rescue
    true
  end unless respond_to?(:closed?)
end