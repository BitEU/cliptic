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
  
  # Load logger first since it's used by other classes
  require_relative "cliptic/version"
  require_relative "cliptic/logger.rb"
  
  class Screen
    extend Curses
    
    def self.setup
      Logger.log_screen_operation("Screen setup started")
      init_curses
      set_colors if has_colors?
      Logger.log_screen_operation("Screen setup completed", {
        lines: lines,
        cols: cols,
        colors: has_colors?
      })
      redraw
    end
    
    def self.init_curses
      Logger.log_screen_operation("Initializing curses", { platform: WINDOWS ? "Windows" : "Unix" })
      init_screen
      
      # Windows-specific terminal settings
      if WINDOWS
        Logger.log_screen_operation("Applying Windows-specific settings")
        # PDCurses specific initialization
        noecho
        cbreak
        curs_set(0)
        # Enable extended characters for box drawing
        Curses.timeout = -1
        # Set code page for proper character display
        system("chcp 65001 > nul") if WINDOWS
      else
        Logger.log_screen_operation("Applying Unix-specific settings")
        raw
        noecho
        curs_set(0)
      end
      Logger.log_screen_operation("Curses initialization completed")
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
      Logger.log_screen_operation("Clearing screen")
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
      Logger.log_screen_operation("Screen redraw requested", { callback: !cb.nil? })
      Interface::Resizer.new.show if Screen.too_small?
      Screen.clear
      cb.call if cb
      Logger.log_screen_operation("Screen redraw completed")
    end
  end
  
  require_relative "cliptic/curses_helper.rb"
  require_relative "cliptic/terminal.rb"
  require_relative "cliptic/lib.rb"
  require_relative "cliptic/config.rb"
  require_relative "cliptic/database.rb"
  require_relative "cliptic/windows.rb"
  require_relative "cliptic/interface.rb"
  require_relative "cliptic/menus.rb"
  require_relative "cliptic/main.rb"
end