module Cliptic
  module Menus
    class Main < Interface::Menu
      def opts
        {
          "Play Today" => ->{
            begin
              Cliptic::Main::Player::Game.new.play
            rescue => e
              puts "Error starting game: #{e.message}"
              puts e.backtrace.join("\n") if $DEBUG
            end
          },
          "Select Date"=> ->{
            begin
              Select_Date.new.choose_opt
            rescue => e
              puts "Error in Select Date: #{e.message}"
              puts e.backtrace.join("\n") if $DEBUG
            end
          },
          "This Week"  => ->{
            begin
              This_Week.new.choose_opt
            rescue => e
              puts "Error in This Week: #{e.message}"
              puts e.backtrace.join("\n") if $DEBUG
            end
          },
          "Recent Puzzles" => ->{
            begin
              Recent_Puzzles.new.choose_opt
            rescue => e
              puts "Error in Recent Puzzles: #{e.message}"
              puts e.backtrace.join("\n") if $DEBUG
            end
          },
          "High Scores"=> ->{
            begin
              High_Scores.new.choose_opt
            rescue => e
              puts "Error in High Scores: #{e.message}"
              puts e.backtrace.join("\n") if $DEBUG
            end
          },
          "Quit"       => ->{exit}
        }
      end
      def title
        "Main Menu"
      end
    end
    class Select_Date < Interface::Menu_With_Stats
      attr_reader :opts, :earliest_date
      def initialize
        @earliest_date = (Date.today << 9) + 1
        set_date(date:Date.today)
        super(height:7, sel:Interface::Date_Selector)
      end
      def title
        "Select Date"
      end
      def ctrls
        base_ctrls = {
          ?j  => ->{inc_date(1)},
          ?k  => ->{inc_date(-1)},
          ?h  => ->{selector.cursor -= 1},
          ?l  => ->{selector.cursor += 1},
          258 => ->{inc_date(1)},   # KEY_DOWN
          259 => ->{inc_date(-1)},  # KEY_UP
          260 => ->{selector.cursor -= 1},  # KEY_LEFT
          261 => ->{selector.cursor += 1},  # KEY_RIGHT
          10  => ->{enter},          # Enter key
          13  => ->{enter},          # Carriage return (Windows)
          ?q  => ->{back},
          3   => ->{back},           # Ctrl+C
          27  => ->{back},           # Escape
          Curses::KEY_RESIZE => ->{Screen.redraw(cb:->{redraw})}
        }
        
        # Add Windows-specific arrow key handling
        if WINDOWS
          base_ctrls.merge!({
            72 => ->{inc_date(-1)},  # Up arrow
            80 => ->{inc_date(1)},   # Down arrow
            75 => ->{selector.cursor -= 1},  # Left arrow
            77 => ->{selector.cursor += 1},  # Right arrow
          })
        end
        
        base_ctrls
      end
      def enter(pre_proc:->{hide}, post_proc:->{show})
        begin
          pre_proc.call if pre_proc
          # Play the game with the selected date
          Main::Player::Game.new(date:stat_date).play
          reset_pos
          post_proc.call if post_proc
        rescue => e
          puts "Error in enter: #{e.message}"
          puts e.backtrace.join("\n") if $DEBUG
          # Try to recover gracefully
          reset_pos
          post_proc.call if post_proc
        end
      end
      
      def back(post_proc:->{hide})
        begin
          selector.stop
          post_proc.call if post_proc
        rescue => e
          puts "Error in back: #{e.message}"
        end
      end

      def stat_date
        Date.new(*@opts.reverse)
      end
      private
      def set_date(date:)
        @opts = [] unless @opts
        @opts[0] = date.day
        @opts[1] = date.month
        @opts[2] = date.year
      end
      def next_date(n)
        @opts.dup.tap{|d| d[selector.cursor] += n }
      end
      def inc_date(n)
        case selector.cursor
        when 0 then inc_day(n)
        when 1 then inc_month(n)
        when 2 then @opts[2] += n
        end
        check_in_range
      end
      def inc_day(n)
        next_date(n).tap do |date|
          if valid?(date)
            @opts[0]+= n
          elsif date[0] == 0
            set_date(date:stat_date-1)
          elsif date[0] > 28
            set_date(date:stat_date+1)
          end
        end
      end
      def inc_month(n)
        next_date(n).tap do |date|
          if valid?(date)
            @opts[1] += n
          elsif date[1] == 0
            set_date(date:stat_date << 1)
          elsif date[1] == 13
            set_date(date:stat_date >> 1)
          elsif date[0] > 28
            set_date(date:last_day_of_month(date:date))
          end
        end
      end
      def valid?(date)
        Date.valid_date?(*date.reverse)
      end
      def last_day_of_month(date:)
        Date.new(date[2], date[1]+1, 1)-1
      end
      def check_in_range
        set_date(date:Date.today)    if date_late?
        set_date(date:earliest_date) if date_early?
      end
      def date_late?
        stat_date > Date.today
      end
      def date_early?
        stat_date < earliest_date
      end
    end
    class This_Week < Interface::Menu_With_Stats
      def days
        @days || 7.times.map{|i| Date.today - i}
      end
      def stat_date
        days[selector.cursor]
      end
      def opts
        @opts || days.each_with_object({}) do |date, hash|
          day_str = date.strftime("%A").ljust(9).center(12) + tickbox(date)
          hash[day_str] = ->{
            hide
            Main::Player::Game.new(date:date).play
            reset_pos
            show
          }
        end
      end
      def title
        "This Week"
      end
      def tickbox(date)
        "[#{Database::State.new(date:date).done ? Chars::Tick : " "}]"
      end
    end
    class Recent_Puzzles < Interface::SQL_Menu_With_Stats
      def initialize
        super(table:Database::Recents)
      end
      def title
        "Recently Played"
      end
    end
    class High_Scores < Interface::SQL_Menu_With_Stats
      def initialize
        super(table:Database::Scores)
      end
      def title
        "High Scores"
      end
    end
  end
end