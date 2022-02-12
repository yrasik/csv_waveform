--cp1251
--[==[
/*
 * This file is part of the "csv_waveform" distribution (https://github.com/yrasik/csv_waveform).
 * Copyright (c) 2022 Yuri Stepanenko.
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





function file_supported()
  return "Comma-Separated Values (*.csv);;\
          Delimiter-Separated Values (*.dsv);;\
          Tab Separated Values (*.tsv);;\
          Lua Table (*.lua)"
end


records = {} -- Глобальный массив для waveform
statist = {}


local function min_max(array)
  local min = 0
  local max = 0
  
  for i = 1, #array do
    if (array[i] > max) then
      max = array[i]
    end
  
    if (array[i] < min) then
      min = array[i]
    end
  end
  
  return min, max
end


local function mean(array)
  local akk = 0

  for i = 1, #array do
    akk = akk + array[i]
  end
  
  akk = akk/#array
  
  return akk
end









function open(file_name)
  local fin = io.open(file_name,"r");
  if ( fin == nil ) then
    --flog:close()
    --os.exit(-1)
    return -1, 'ERROR: Open file'
  end

  for l in fin:lines() do 
    local fields = trim(l)
    fields = fields:gsub('%s+', '')
    fields = split(fields, ',')

    if( (fields[1] ~= nil) and (fields[2] ~= nil) ) then
      fields[1] = tonumber(fields[1])
      fields[2] = tonumber(fields[2])
      if(type(fields[2]) == 'number') then
        records[#records + 1] = {fields[1], fields[2]}
      end
    end
  end

  fin:close()
  return 0, #records
end

function get_record(num)
  return records[num][1], records[num][2]
end








