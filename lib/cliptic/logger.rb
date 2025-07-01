module Cliptic
  module Logger
    # Windows-compatible log directory
    LOG_DIR = if WINDOWS
      "#{ENV['APPDATA']}\\cliptic\\logs"
    else
      "#{Dir.home}/.config/cliptic/logs"
    end
    
    LOG_FILE = if WINDOWS
      "#{LOG_DIR}\\ux_debug.log"
    else
      "#{LOG_DIR}/ux_debug.log"
    end
    
    # Enable UX logging with environment variable or config
    UX_LOGGING_ENABLED = ENV['CLIPTIC_UX_DEBUG'] == '1' || $config&.dig(:ux_debug)
    
    class << self
      def init
        return unless UX_LOGGING_ENABLED
        FileUtils.mkdir_p(LOG_DIR) unless Dir.exist?(LOG_DIR)
        
        # Rotate log if it gets too large (>5MB)
        if File.exist?(LOG_FILE) && File.size(LOG_FILE) > 5 * 1024 * 1024
          File.rename(LOG_FILE, "#{LOG_FILE}.old")
        end
        
        # Log session start
        log_raw("\n" + "="*80)
        log_raw("NEW CLIPTIC SESSION STARTED: #{Time.now}")
        log_raw("Platform: #{RUBY_PLATFORM}")
        log_raw("Terminal size: #{Curses.lines}x#{Curses.cols}") if defined?(Curses.lines)
        log_raw("="*80)
      end
      
      def log_ux(category, event, details = {})
        return unless UX_LOGGING_ENABLED
        
        timestamp = Time.now.strftime("%H:%M:%S.%3N")
        thread_id = Thread.current.object_id.to_s(16)[-4..-1]
        
        details_str = details.empty? ? "" : " | #{format_details(details)}"
        
        message = "[#{timestamp}][#{thread_id}][#{category.upcase}] #{event}#{details_str}"
        log_raw(message)
      end
      
      def log_keypress(key, key_name = nil, context = nil)
        details = {
          key_code: key.is_a?(Integer) ? key : key.ord,
          key_char: key.is_a?(Integer) ? (key < 256 ? key.chr : "SPECIAL") : key,
          key_name: key_name,
          context: context
        }.compact
        
        log_ux("KEYPRESS", "Key pressed", details)
      end
      
      def log_selection_change(from, to, context = nil)
        details = {
          from: from,
          to: to,
          context: context
        }.compact
        
        log_ux("SELECTION", "Selection changed", details)
      end
      
      def log_menu_draw(menu_name, options = nil, selected = nil)
        details = {
          menu: menu_name,
          option_count: options&.size,
          selected_option: selected
        }.compact
        
        log_ux("MENU", "Menu drawn", details)
      end
      
      def log_window_operation(operation, window_class, details = {})
        details = details.merge(window: window_class)
        log_ux("WINDOW", operation, details)
      end
      
      def log_cursor_move(from_pos, to_pos, context = nil)
        details = {
          from: format_position(from_pos),
          to: format_position(to_pos),
          context: context
        }.compact
        
        log_ux("CURSOR", "Cursor moved", details)
      end
      
      def log_screen_operation(operation, details = {})
        log_ux("SCREEN", operation, details)
      end
      
      def log_game_state(state, details = {})
        log_ux("GAME", "State: #{state}", details)
      end
      
      def flush
        return unless UX_LOGGING_ENABLED && @log_file
        @log_file.flush
      end
      
      private
      
      def log_raw(message)
        File.open(LOG_FILE, "a") do |f|
          f.puts(message)
          f.flush
        end
      rescue => e
        # Silently ignore logging errors to avoid disrupting the main application
        STDERR.puts "Logging error: #{e.message}" if $DEBUG
      end
      
      def format_details(details)
        details.map do |key, value|
          formatted_value = case value
          when String
            value.length > 50 ? "#{value[0..47]}..." : value
          when Array
            "[#{value.size} items]"
          when Hash
            "{#{value.size} keys}"
          else
            value.to_s
          end
          "#{key}=#{formatted_value}"
        end.join(", ")
      end
      
      def format_position(pos)
        case pos
        when Array
          "#{pos[0]},#{pos[1]}"
        when Hash
          "#{pos[:line] || pos[:y]},#{pos[:col] || pos[:x]}"
        else
          pos.to_s
        end
      end
    end
  end
end
