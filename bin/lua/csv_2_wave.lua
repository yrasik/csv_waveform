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



local csv_2_wave = {}

--dofile("lua/wav.lua")


local function version()
  return 'v 1.0a'
end


local function info()
  local info = 
[[
  Модуль для извлечения сигнала из ngspice csv - файлов и сохранения в формате wave.
  Выполняет:
    1) Загружает файл со строчками вида
-------------------------------
 time           "OUT"         
 0,0000000e+00 -1,9470244e+00 
 6,2500000e-07 -1,9470630e+00 
 6,3168713e-07 -1,9470635e+00 
-------------------------------
   2) Создаёт wave - файл в формате моно, 16 бит 48000 Гц.

  Внимание ! Программа ngspice выдаёт таблицу с переменным шагом дискретизации.
  Возможно нужно добавить методы ЦОС перед децимацией (вычисление минимального шага,
  экстраполяцию, фильтрацию и в заключение - децимацию). Из - за того, что шаг мелкий,
  компьютеру не хватает ресурсов. Нужно применять потоковые адгоритмы для этого.
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


local function exec(fin, fout_name, flog)
  local records = {}
  local writer = wav.create_context(fout_name, "w")  

  for l in fin:lines() do 
    local fields = trim(l)
    fields = fields:gsub('%s+', '\t')
    fields = split(fields, '\t')

    if( (fields[1] ~= nil) and (fields[2] ~= nil) ) then
      fields[1] = fields[1]:gsub(",", ".")
      fields[1] = tonumber(fields[1])
      fields[2] = fields[2]:gsub(",", ".")
      fields[2] = tonumber(fields[2])
      if(type(fields[2]) == 'number') then
        records[#records + 1] = {time = fields[1], sample = fields[2]}
      end
    end
  end

  local timestep = records[2].time - records[1].time
  local samplerate = 1/timestep
  print('samplerate =', samplerate)
  
  if( (47000 < samplerate) and (samplerate < 49000) ) then  --FIXME
    writer.init(1, 48000, 16)
  elseif( (7900 < samplerate) and (samplerate < 8100) ) then
    writer.init(1, 8000, 16)
  end
  
  
  
  
  




  local time = 0
  local s = {}

  print('#records = '..#records)
  
  local i = 1
  while i < #records do
    if( records[i].time > time ) then
      s[#s + 1] = records[i].sample
      time = time + timestep
      --print(time)
    end
    i = i + 1
  end
  

  local akk = 0
  for i = 1, #s do 
    akk = akk + s[i] 
  end
  local mid = akk/#s

  print('mid = '..mid)
  for i = 1, #s do 
    --s[i] = (s[i] - mid ) * 32767  --FIXME
    s[i] = (s[i] - mid ) * 2^13  --FIXME
  end
 
  writer.write_samples_interlaced(s)
  writer.finish()
end





------------ main ------------

if arg[0]:find('csv_2_wave.lua') ~= nil then
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


