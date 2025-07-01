module Cliptic
  module Terminal
    class Command
      def self.run
        begin
          Logger.log_ux("TERMINAL", "Cliptic started", {
            args: ARGV.join(" "),
            platform: RUBY_PLATFORM
          })
          ARGV.size > 0 ? parse_args : main_menu
        rescue Interrupt
          Logger.log_ux("TERMINAL", "Interrupted by user")
          cleanup_and_exit(nil, "Exiting cliptic...")
        rescue StandardError => e
          Logger.log_ux("TERMINAL", "Fatal error", { error: e.message })
          cleanup_and_exit(e)
        end
      end
      
      private
      
      def self.parse_args
        case ARGV[0]
        when "today"
          play_today
        when "reset"
          Reset_Stats.route.call
        else
          puts "Unknown command: #{ARGV[0]}"
          puts "Available commands: today, reset"
        end
      end
      
      def self.play_today
        setup
        offset = ARGV[1]&.to_i || 0
        date = Date.today + offset
        Cliptic::Main::Player::Game.new(date: date).play
      rescue => e
        cleanup_and_exit(e)
      end
      
      def self.cleanup_and_exit(error, message = nil)
        begin
          Curses.close_screen if Curses.stdscr && !Curses.closed?
        rescue
          # Ignore errors during cleanup
        end
        
        if message
          puts message
        elsif error
          puts "Error: #{error.message}"
          puts "Backtrace:" if $DEBUG
          puts error.backtrace if $DEBUG
        end
        
        exit(error ? 1 : 0)
      end
      
      def self.setup
        begin
          Logger.log_ux("TERMINAL", "Setup started")
          Config::Default.set
          Screen.setup
          Config::Custom.set
          Logger.log_ux("TERMINAL", "Setup completed")
          at_exit { close }
        rescue => e
          Logger.log_ux("TERMINAL", "Setup error", { error: e.message })
          puts "Setup error: #{e.message}"
          raise
        end
      end
      
      def self.main_menu
        Logger.log_ux("TERMINAL", "Starting main menu")
        setup
        Cliptic::Menus::Main.new.choose_opt
      rescue => e
        Logger.log_ux("TERMINAL", "Error in main menu", { error: e.message })
        cleanup_and_exit(e)
      end
      
      def self.close
        begin
          if Curses.stdscr && !Curses.closed?
            Curses.close_screen
            puts "Thanks for playing!"
          end
        rescue => e
          puts "Thanks for playing!"
        end
      end
    end
    
    class Reset_Stats
      def self.route
        if valid_options.include?(c = ARGV.shift)
          ->{confirm_reset(c)}
        else 
          ->{puts "Unknown option #{c}"}
        end
      end
      
      private
      def self.valid_options
        ["scores", "all", "states", "recents"]
      end
      
      def self.confirm_reset(table)
        puts prompt(table)
        user_confirmed? ?
          reset(table) : 
          puts("Wise choice")
      end
      
      def self.prompt(table)
        <<~prompt
        cliptic: Reset #{table}
        Are you sure? This cannot be undone! [Y/n]
        prompt
      end
      
      def self.user_confirmed?
        gets.chomp == "Y"
      end
      
      def self.reset(table)
        table == "all" ?
          Database::Delete.all :
          Database::Delete.table(table)
      end
    end
  end
end