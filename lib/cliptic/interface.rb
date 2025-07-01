module Cliptic
  module Interface
    class Top_Bar < Windows::Bar
      attr_reader :date
      def initialize(date:Date.today)
        super(line:0)
        @date = date
      end
      def draw
        super
        add_str(x:1, str:title, bold:true)
        add_str(x:title.length+2, str:date.to_long)
      end
      def title
        "cliptic:"
      end
      def reset_pos
        move(line:0, col:0)
      end
    end
    class Bottom_Bar < Windows::Bar
      def initialize
        super(line:Curses.lines-1)
      end
      def draw
        super
        noutrefresh
      end
      def reset_pos
        move(line:Curses.lines-1, col:0)
      end
    end
    class Logo < Windows::Grid
      attr_reader :text
      def initialize(line:, text:"CLIptic")
        super(y:1, x:text.length, line:line)
        @text = text
      end
      def draw(cp_grid:$colors[:logo_grid], 
               cp_text:$colors[:logo_text], bold:true)
        super(cp:cp_grid)
        bold(bold).color(cp_text)
        add_str(str:text)
        reset_attrs
        refresh
      end
    end
    class Menu_Box < Windows::Window
      include Interface
      attr_reader :logo, :title, :top_b, :bot_b, :draw_bars
      def initialize(y:, title:false)
        @logo = Logo.new(line:line+1)
        @title = title
        super(y:y, x:logo.x+4, line:line, col:nil)
        @top_b = Top_Bar.new
        @bot_b = Bottom_Bar.new
        @draw_bars = true
      end
      def draw
        Logger.log_window_operation("Drawing menu box", self.class.name, {
          title: @title,
          draw_bars: @draw_bars,
          line: line,
          dimensions: "#{y}x#{x}"
        })
        super
        [top_b, bot_b].each(&:draw) if draw_bars
        logo.draw
        add_title if title
        Logger.log_window_operation("Menu box draw completed", self.class.name)
        self
      end
      def line
        (Curses.lines-15)/2
      end
      def add_title(y:4, str:title, cp:$colors[:title], bold:true)
        add_str(y:y, str:str, cp:cp, bold:bold)
      end
      def reset_pos
        move(line:line)
        logo.move(line:line+1)
        [top_b, bot_b].each(&:reset_pos)
      end
    end
    class Resizer < Menu_Box
      def initialize
        super(y:8, title:title)
      end
      def title
        "Screen too small"
      end
      def draw
        Screen.clear
        reset_pos
        super
        wrap_str(str:prompt, line:5)
        refresh
      end
      def show
        while Screen.too_small?
          draw
          exit if (c = getch) == ?q || c == 3
        end
      end
      def prompt
        "Screen too small. Increase screen size to run cliptic."
      end
      def line
        (Curses.lines-8)/2
      end
    end
    class Selector < Windows::Window
      attr_reader :opts, :ctrls, :run, :tick
      attr_accessor :cursor
      
      def initialize(opts:, ctrls:, x:, line:,
                     tick:nil, y:opts.length, col:nil)
        super(y:y, x:x, line:line, col:col)
        @opts, @ctrls, @tick = opts, ctrls, tick
        @cursor, @run = 0, true
      end
      
      def select
        Logger.log_ux("SELECTOR", "Starting selection loop", {
          options_count: opts.length,
          initial_cursor: cursor
        })
        
        while @run
          begin
            draw
            key = getch
            Logger.log_keypress(key, nil, "Selector")
            
            # Handle Windows-specific key processing
            key = process_windows_key(key) if WINDOWS
            action = ctrls[key]
            
            if action
              Logger.log_ux("SELECTOR", "Executing key action", {
                key: key.is_a?(Integer) ? key : key.ord,
                cursor_before: cursor
              })
              action.call
              Logger.log_ux("SELECTOR", "Key action completed", {
                cursor_after: cursor
              })
            else
              Logger.log_ux("SELECTOR", "No action for key", { key: key })
            end
          rescue => e
            Logger.log_ux("SELECTOR", "Error in selection loop", {
              error: e.message,
              cursor: cursor
            })
            # Log error and continue instead of crashing
            puts "Error in selector: #{e.message}" if $DEBUG
            puts e.backtrace.join("\n") if $DEBUG
          end
        end
        
        Logger.log_ux("SELECTOR", "Selection loop ended", { final_cursor: cursor })
      rescue => e
        Logger.log_ux("SELECTOR", "Fatal error in selector", { error: e.message })
        puts "Fatal error in selector: #{e.message}"
        @run = false
      end
      
      def stop
        @run = false
      end
      
      def cursor=(n)
        old_cursor = @cursor
        @cursor = Pos.wrap(val:n, min:0, max:opts.length-1)
        Logger.log_selection_change(old_cursor, @cursor, "Selector cursor")
      end
      
      private
      
      def process_windows_key(key)
        return key unless WINDOWS
        
        case key
        when 224  # Extended key prefix in Windows
          ext_key = getch rescue nil
          return nil unless ext_key
          case ext_key
          when 72 then 259  # Up arrow -> Curses::KEY_UP
          when 80 then 258  # Down arrow -> Curses::KEY_DOWN  
          when 75 then 260  # Left arrow -> Curses::KEY_LEFT
          when 77 then 261  # Right arrow -> Curses::KEY_RIGHT
          else ext_key
          end
        when 0    # Another extended key prefix
          ext_key = getch rescue nil
          return nil unless ext_key
          case ext_key
          when 72 then 259  # Up arrow
          when 80 then 258  # Down arrow
          else ext_key
          end
        else
          key
        end
      end
      
      def draw
        begin
          Logger.log_window_operation("Drawing selector", self.class.name, {
            cursor: cursor,
            options_count: opts.length,
            current_option: opts[cursor]
          })
          
          Curses.curs_set(0)
          setpos
          opts.each_with_index do |opt, i|
            standout if cursor == i
            self << format_opt(opt)
            standend
          end
          tick.call if tick
          refresh
          
          Logger.log_window_operation("Selector draw completed", self.class.name)
        rescue => e
          Logger.log_ux("SELECTOR", "Error in draw", { error: e.message })
          puts "Error in draw: #{e.message}" if $DEBUG
        end
      end
      
      def format_opt(opt)
        opt.to_s.center(x)
      end
    end
    class Date_Selector < Selector
      def initialize(opts:, ctrls:, line:, x:18, tick:)
        super(y:1, x:18, opts:opts, 
              ctrls:ctrls, line:line, tick:tick)
      end
      def format_opt(opt)
        opt.to_s.rjust(2, "0").center(6)
      end
    end
    class Stat_Window < Windows::Window
      def initialize(y:5, x:33, line:)
        super(y:y, x:x, line:line)
      end
      def show(date:, cp:$colors[:stats])
        draw(clr:true).color(cp)
        get_stats(date:date).each_with_index do |line, i|
          setpos(i+1, 8)
          self << line
        end
        color.noutrefresh
      end
      def get_stats(date:)
        Database::Stats.new(date:date).stats_str
      end
    end
    class Menu < Menu_Box
      attr_reader :selector, :height
      def initialize(height:opts.length+6,
                     sel:Selector, sel_opts:opts.keys,
                     tick:nil, **)
        super(y:height, title:title)
        @height = height
        @selector = sel.new(opts:sel_opts, ctrls:ctrls, line:line+5, x:logo.x, tick:tick)
      end
      def choose_opt
        Logger.log_menu_draw("Menu selection started", opts.keys, selector.cursor)
        show
        selector.select
        Logger.log_menu_draw("Menu selection ended", opts.keys, selector.cursor)
      end
      def enter(pre_proc:->{hide}, post_proc:->{show})
        begin
          Logger.log_ux("MENU", "Enter action triggered", {
            selected_option: opts.keys[selector.cursor],
            cursor: selector.cursor
          })
          
          pre_proc.call if pre_proc
          action = opts.values[selector.cursor]
          if action
            Logger.log_ux("MENU", "Executing menu action", {
              option: opts.keys[selector.cursor]
            })
            action.call
            Logger.log_ux("MENU", "Menu action completed")
          else
            Logger.log_ux("MENU", "No action found for cursor position", {
              cursor: selector.cursor
            })
            puts "No action found for cursor position #{selector.cursor}" if $DEBUG
          end
          reset_pos
          post_proc.call if post_proc
        rescue => e
          Logger.log_ux("MENU", "Error in menu enter", {
            error: e.message,
            cursor: selector.cursor
          })
          puts "Error in menu enter: #{e.message}"
          puts e.backtrace.join("\n") if $DEBUG
          # Try to recover
          reset_pos
          post_proc.call if post_proc
        end
      end
      def back(post_proc:->{hide})
        Logger.log_ux("MENU", "Back action triggered", { cursor: selector.cursor })
        selector.stop
        post_proc.call if post_proc
      end
      def show
        draw
      end
      def hide
        clear
      end
      def ctrls
        base_ctrls = {
          ?j  => ->{
            Logger.log_keypress('j', 'down_vim', 'Menu navigation')
            selector.cursor += 1
          },
          ?k  => ->{
            Logger.log_keypress('k', 'up_vim', 'Menu navigation')
            selector.cursor -= 1
          },
          258 => ->{
            Logger.log_keypress(258, 'down_arrow', 'Menu navigation')
            selector.cursor += 1
          },
          259 => ->{
            Logger.log_keypress(259, 'up_arrow', 'Menu navigation')
            selector.cursor -= 1
          },
          10  => ->{
            Logger.log_keypress(10, 'enter', 'Menu selection')
            enter
          },
          13  => ->{
            Logger.log_keypress(13, 'carriage_return', 'Menu selection')
            enter
          },  # Carriage return for Windows
          ?q  => ->{
            Logger.log_keypress('q', 'quit', 'Menu exit')
            back
          },
          3   => ->{
            Logger.log_keypress(3, 'ctrl_c', 'Menu exit')
            back
          },
          27  => ->{
            Logger.log_keypress(27, 'escape', 'Menu exit')
            back
          },   # Escape key
          Curses::KEY_RESIZE => 
            ->{
              Logger.log_screen_operation("Window resize detected in menu")
              Screen.redraw(cb:->{redraw})
            }
        }
        
        # Add Windows-specific arrow key handling
        if WINDOWS
          base_ctrls.merge!({
            72 => ->{
              Logger.log_keypress(72, 'windows_up_arrow', 'Menu navigation')
              selector.cursor -= 1
            },  # Up arrow
            80 => ->{
              Logger.log_keypress(80, 'windows_down_arrow', 'Menu navigation')
              selector.cursor += 1
            },  # Down arrow
          })
        end
        
        base_ctrls
      end
      def reset_pos
        super
        selector.move(line:line+5)
      end
      def redraw
        reset_pos
        draw
      end
    end
    class Menu_With_Stats < Menu
      attr_reader :stat_win
      def initialize(height:opts.length+6, sel:Selector, **)
        super(height:height, sel:sel, sel_opts:opts, 
              tick:->{update_stats})
        @stat_win = Stat_Window.new(line:line+height)
      end
      def update_stats
        stat_win.show(date:stat_date)
      end
      def hide
        super
        stat_win.clear
      end
      def enter
        hide
        Main::Player::Game.new(date:stat_date).play
        reset_pos
        show
      end
      def reset_pos
        super
        stat_win.move(line:line+height)
      end
    end
    class SQL_Menu_With_Stats < Menu_With_Stats
      include Database
      attr_reader :dates
      def initialize(table:)
        @dates = []
        begin
          @dates = table.new
            .select_list.map{|d| Date.parse(d["date"])}
        rescue => e
          puts "Error loading #{table}: #{e.message}" if $DEBUG
          # Return empty dates array if table doesn't exist
        end
        super
      end
      def opts
        if dates.empty?
          {"No entries yet" => nil}
        else
          dates.each_with_object({}) do |date, hash|
            hash[date.to_long] = ->{
              hide
              Main::Player::Game.new(date:date).play
              reset_pos
              show
            }
          end
        end
      end
      def stat_date
        dates.empty? ? Date.today : dates[selector.cursor]
      end
    end
    class Yes_No_Menu < Menu
      attr_reader :yes, :no, :post_proc
      def initialize(yes:, no:->{back}, post_proc:nil, title:nil)
        @title = title
        @yes, @no, @post_proc = yes, no, post_proc
        super
      end
      def opts
        {
          "Yes" => ->{yes.call; back; post},
          "No"  => ->{no.call; post}
        }
      end
      def post
        post_proc.call if post_proc
      end
    end
  end
end