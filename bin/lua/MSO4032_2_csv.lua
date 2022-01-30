--cp1251
--[==[
/*
 * This file is part of the "SystemC_AMS_Lua" distribution (https://github.com/yrasik/SystemC_AMS_Lua).
 * Copyright (c) 2021 Yuri Stepanenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
]==]

local wav = require("wav")



local MSO4032_2_csv = {}

--dofile("lua/wav.lua")


local function version()
  return 'v 1.0a'
end


local function info()
  local info = 
[[

]]
  return info
end


local function process_args()
  -- get args set by user in command line
  local t, i = {}, 1
  while i < #arg do
    local a = arg[i]
    if a == "--fin" then
      t.fin_name = arg[i + 1]
      i = i + 2
    elseif a == "--fout" then
      t.fout_name = arg[i + 1]
      i = i + 2
    elseif a == "--flog" then
      t.flog_name = arg[i + 1]
      i = i + 2
    elseif a == "--size" then
      t.size = arg[i + 1]
      i = i + 2
    else
      print(usage.."Bad flag: "..a)
      os.exit(-1)
    end
  end
  return t
end


local function split(instr, sep)
  local t = {}
  for str in string.gmatch(instr, "([^"..sep.."]+)") do
    table.insert(t, str)
  end
  return t
end  


local function replace(str, what, with)
  what = what:gsub("[%(%)%.%+%-%*%?%[%]%^%$%%]", "%%%1")
  with = with:gsub("[%%]", "%%%%")
  return str:gsub(what, with)
end  


local function trim(instr)
  return string.gsub(instr, "^%s*(.-)%s*$", "%1")
end



local function MSO4032_to_signal(fin, flog)
  local records = {}
  
  for l in fin:lines() do 
    local flag = l:match('^[%-%d]')
  
    if( flag ~= nil ) then
      local fields = trim(l)
      fields = fields:gsub('[,%s]+', '\t')
      fields = split(fields, '\t')
      if( (fields[1] ~= nil) and (fields[2] ~= nil) ) then
        fields[1] = tonumber(fields[1])
        fields[2] = tonumber(fields[2])
        if(type(fields[2]) == 'number') then
          records[#records + 1] = {time = fields[1], sample = fields[2]}
        end
      end
    end  
  end
  
  local time_null = records[1].time
  for i = 1, #records do
    records[i].time = records[i].time - time_null
  end
  
  return records
end


local function exec(fin, fout_name, flog)
  local records = MSO4032_to_signal(fin, flog)

  local fout = io.open(fout_name, "w");
  if ( fout == nil ) then
    return
  end
  
  for i = 1, #records do
    fout:write(string.format('%e\t%e\n',records[i].time, records[i].sample))
  end

  fout:close()
end


------------ main ------------

if arg[0]:find('MSO4032_2_csv.lua') ~= nil then
  local args = process_args()


  local flog = io.open(args.flog_name,"w");
  if ( flog == nil ) then
    fout:close()
    os.exit(-1)
  end


  local fin = io.open(args.fin_name,"r");
  if ( fin == nil ) then
    flog:close()
    os.exit(-1)
  end

  exec(fin, args.fout_name, flog)

  fin:close()
  flog:close()

  os.exit(0)
end


return {
  version = version,
  info = info,
  exec = exec
}


