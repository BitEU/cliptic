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
      # Reduced minimum requirements for better Windows compatibility
      # Original: lines < 36 || cols < 61
      # New: More reasonable minimums that work with standard terminal sizes
      if WINDOWS
        # Even more relaxed for Windows conhost
        lines < 25 || cols < 80
      else
        # Slightly relaxed for other platforms
        lines < 30 || cols < 80
      end
    end
    
    def self.redraw(cb:nil)
      Interface::Resizer.new.show if Screen.too_small?
      Screen.clear
      cb.call if cb
    end
  end
  
  require_relative "cliptic/version"
  require_relative "cliptic/curses_helper"
  require_relative "cliptic/terminal"
  require_relative "cliptic/lib"
  require_relative "cliptic/config"
  require_relative "cliptic/database"
  require_relative "cliptic/windows"
  require_relative "cliptic/interface"
  require_relative "cliptic/menus"
  require_relative "cliptic/main"
end