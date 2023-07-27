describe 'database' do
    def run_script(commands)
      raw_output = nil
      IO.popen("./main", "r+") do |pipe|
        commands.each do |command|
          pipe.puts command
        end
  
        pipe.close_write
  
        # Read entire output
        raw_output = pipe.gets(nil)
      end
      raw_output.split("\n")
    end
  
    it 'allows inserting strings that are the maximum length' do
        long_username = "a"*32
        long_email = "a"*255
        script = [
          "insert 1 #{long_username} #{long_email}",
          "select",
          ".exit",
        ]
        result = run_script(script)
        expect(result).to match_array([
          "db > Executed.",
          "db > (1, #{long_username}, #{long_email})",
          "Executed.",
          "db > ",
        ])
      end
  end