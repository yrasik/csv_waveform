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













local function open_csv(file_name, flog)
  local fin = io.open(file_name,"r");
  if ( fin == nil ) then
    local err = 'ERROR: Open file'
    flog:write(err..'\n')
    return -1, err
  end

  local columns = 0

  for l in fin:lines() do 
    local fields = trim(l)
    fields = fields:gsub('%s+', '')
    fields = split(fields, ',')

    if( (fields[1] ~= nil) and 
        (fields[2] ~= nil) and
        (fields[3] ~= nil)
      ) then
      fields[1] = tonumber(fields[1])
      fields[2] = tonumber(fields[2])
      fields[3] = tonumber(fields[3])
      if( (type(fields[1]) == 'number') and 
          (type(fields[2]) == 'number') and
          (type(fields[3]) == 'number')
        ) then
        if (columns == 0) then
          columns = 3
        elseif (columns ~= 3) then
          local err = 'ERROR: 3 In csv - file structure'
          flog:write(err..'\n')
          return -2, err
        end
        records[#records + 1] = {fields[1], fields[2], fields[3]}
      end
      goto continue
    end

    if( (fields[1] ~= nil) and 
        (fields[2] ~= nil)
      ) then
      fields[1] = tonumber(fields[1])
      fields[2] = tonumber(fields[2])
      if( (type(fields[1]) == 'number') and 
          (type(fields[2]) == 'number')
        ) then
        if (columns == 0) then
          columns = 2
        elseif (columns ~= 2) then
          local err = 'ERROR: 2 In csv - file structure'
          flog:write(err..'\n')
          return -2, err
        end        
        records[#records + 1] = {fields[1], fields[2]}
      end
      goto continue
    end
    
    
    ::continue::
  end
  fin:close()
  return 0, #records, #records[1]
end




------------------------ Global Functions ---------------------------

function open(full_file_name)
  local flog = io.open("../log/lua.log","w");
  if ( flog == nil ) then
    return -1, 'ERROR: Open log file'
  end


  if (type(full_file_name) ~= 'string' ) then
    local err = 'ERROR: File name is empty'
    flog:write(err..'\n')
    flog:close()
    return -1, err
  end  
  
  local ext = full_file_name:match('^[%D%d]+%.([%D%d]+)$')

  if (type(ext) ~= 'string' ) then
    local err = 'ERROR: File extention is empty'
    flog:write(err..'\n')
    flog:close()
    return -2, err
  end  
  
  ext = string.lower(ext)

  if (ext == 'csv') then
    return open_csv(full_file_name, flog)
  elseif (ext == 'dsv') then
    local err = 'ERROR: File extention is unknown'
    flog:write(err..'\n')
    flog:close()
    return -4, err
  else
    local err = 'ERROR: File extention is unknown'
    flog:write(err..'\n')
    flog:close()
    return -3, err
  end 

  local err = 'ERROR: Unknown'
  flog:write(err..'\n')
  flog:close()
  return -5, 'ERROR: Unknown'
end


function get_record(num)
  return records[num][1], records[num][2], records[num][3]
end


local function min_max1(array)
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


