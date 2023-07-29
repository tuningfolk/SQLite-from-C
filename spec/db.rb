describe 'database' do
  before do
    `rm -rf test.db`
  end



    def run_script(commands)
      raw_output = nil
      IO.popen("./main test.db", "r+") do |pipe|
        commands.each do |command|
          pipe.puts command
        end
  
        pipe.close_write
  
        # Read entire output
        raw_output = pipe.gets(nil)
      end
      raw_output.split("\n")
    end

    it 'keeps data after closing connection' do
      result1 = run_script([
        "insert 1 user1 person1@example.com",
        ".exit"
      ])
      expect(result1).to match_array([
        "db > Executed.",
        "db > ",
      ])

      result2 = run_script([
        "select",
        ".exit",
      ])
      expect(result2).to match_array([
        "db > (1, user1, person1@example.com)",
        "Executed.",
        "db > ",
      ])
    end
  
    it 'prints error message if strings are too long' do
      long_username = "a"*33
      long_email = "a"*256
      script = [
        "insert 1 #{long_username} #{long_email}",
        "select",
        ".exit",
      ]
      result = run_script(script)
      expect(result).to match_array([
        "db > String is too long.",
        "db > Executed.",
        "db > ",
      ])
    end
    it 'keeps data after closing connection' do
      result1 = run_script([
        "insert 1 user1 person1@example.com",
        ".exit",
      ])
      expect(result1).to match_array([
        "db > Executed.",
        "db > ",
      ])
      result2 = run_script([
        "select",
        ".exit",
      ])
      expect(result2).to match_array([
        "db > (1, user1, person1@example.com)",
        "Executed.",
        "db > ",
      ])
    end
  end