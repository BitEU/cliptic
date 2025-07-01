module Cliptic
  module Terminal
    class Command
      def self.run
        begin
          ARGV.size > 0 ? parse_args : main_menu
        rescue Interrupt
          puts "\nExiting cliptic..."
        rescue StandardError => e
          cleanup_and_exit(e)
        end
      end
      
      private
      
      def self.cleanup_and_exit(error)
        begin
          Curses.close_screen if Curses.stdscr
        rescue
          # Ignore errors during cleanup
        end
        
        puts "Error: #{error.message}"
        puts "Backtrace:" if $DEBUG
        puts error.backtrace if $DEBUG
        exit(1)
      end
      
      def self.setup
        begin
          Config::Default.set
          Screen.setup
          Config::Custom.set
          at_exit { close }
        rescue => e
          puts "Setup error: #{e.message}"
          raise
        end
      end
      
      def self.main_menu
        setup
        Cliptic::Menus::Main.new.choose_opt
      rescue => e
        cleanup_and_exit(e)
      end
      
      def self.close
        begin
          if Curses.stdscr && !Curses.isendwin
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
        gets.chomp === "Y"
      end
      def self.reset(table)
        table == "all" ?
          Database::Delete.all :
          Database::Delete.table(table)
      end
    end
  end
end

