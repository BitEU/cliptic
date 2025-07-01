module Cliptic
  require 'cgi'
  require 'net/http'
  require 'uri'
  require 'curses'
  require 'date'
  require 'fileutils'
  require 'json'
  require 'sqlite3'
  require 'time'
  
  # Windows compatibility check
  WINDOWS = Gem.win_platform?
  
  class Screen
    extend Curses
    
    def self.setup
      init_curses
      set_colors if has_colors?
      redraw
    end
    
    def self.init_curses
      init_screen
      
      # Windows-specific terminal settings
      if WINDOWS
        # PDCurses specific initialization
        noecho
        cbreak
        curs_set(0)
        # Enable extended characters for box drawing
        Curses.timeout = -1
        # Set code page for proper character display
        system("chcp 65001 > nul") if WINDOWS
      else
        raw
        noecho
        curs_set(0)
      end
    end
    
    def self.set_colors
      start_color
      use_default_colors
      1.upto(8) do |i|
        init_pair(i, i, -1)
        init_pair(i+8, 0, i)
      end
    end
    
    def self.clear
      stdscr.clear
      stdscr.refresh
    end
    
    def self.too_small?
      lines < 36 || cols < 61
    end
    
    def self.redraw(cb:nil)
      Interface::Resizer.new.show if Screen.too_small?
      Screen.clear
      cb.call if cb
    end
  end
  
  require_relative "cliptic/version"
  require_relative "cliptic/terminal.rb"
  require_relative "cliptic/lib.rb"
  require_relative "cliptic/config.rb"
  require_relative "cliptic/database.rb"
  require_relative "cliptic/windows.rb"
  require_relative "cliptic/interface.rb"
  require_relative "cliptic/menus.rb"
  require_relative "cliptic/main.rb"
end