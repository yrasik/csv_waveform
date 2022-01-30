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

local wave_2_ngspice = {}



local function version()
  return 'v 1.1a'
end


local function info()
  local info = 
[[
  Модуль для извлечения сигнала из wave - файлов и сохранения в формате ngspice.
  Выполняет:
    1) Извлекает из wave - файла первую дорожку;
    2) Создаёт текстовый файл подобного содержания:
-------------------------------------
0.000000	0.083687
0.062500	0.057063
0.125000	0.011906
0.187500	-0.013437
...
...
7499.875000	-0.000719
7499.937500	-0.000594
   
-------------------------------------
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


local function ArbExpress_2_9_2013_csv_fout(fout_name, signal, flog)
  local fout_name = fout_name..'_ArbExpress_2_9_2013'..'.csv'
  local fout = io.open(fout_name,"w");
  if ( fout == nil ) then
    flog:write(string.format('ERROR:  "%s"\n', fout_name))
  end
   
  local clock = 1/(signal[2][1] - signal[1][1])
  fout:write(string.format("#CLOCK=%e\n", clock)) 
  fout:write(string.format("#SIZE=%d\n", #signal))

  for i = 1, #signal do
    fout:write(string.format("%f,0,0\n", signal[i][2]))
  end

  fout:close()
end


local function exec(fin_name, fout, flog, size)
  local signal = {}
  reader = wav.create_context(fin_name, "r")
  print("Filename: " .. reader.get_filename())
  print("Mode: " .. reader.get_mode())
  print("File size: " .. reader.get_file_size())
  print("Channels: " .. reader.get_channels_number())
  print("Sample rate: " .. reader.get_sample_rate())
  print("Byte rate: " .. reader.get_byte_rate())
  print("Block align: " .. reader.get_block_align())
  print("Bitdepth: " .. reader.get_bits_per_sample())
  print("Samples per channel: " .. reader.get_samples_per_channel())
  print("Sample at 500ms: " .. reader.get_sample_from_ms(500))
  print("Milliseconds from 3rd sample: " .. reader.get_ms_from_sample(3))
  print(string.format("Min- & maximal amplitude: %d <-> %d", reader.get_min_max_amplitude()))
 -- reader.set_position(256)
 -- print("Sample 256, channel 2: " .. reader.get_samples(1)[2][1])

  -- Get first frequencies
  reader.set_position(0)
  print("--->")

  local time = 0
  local timestep_ms = reader.get_ms_from_sample(4) - reader.get_ms_from_sample(3)
  print('timestep_ms = '..timestep_ms)
  local size_tmp = reader.get_samples_per_channel()
    
  if ( type(size) ~= 'number' ) then
    size = size_tmp
  else
    if (size > size_tmp) then
      size = size_tmp
    end
  end

  local timestep_s = timestep_ms * 10^-3
  for i = 1, size do
    local sample = reader.get_samples(1)[1]
    --fout:write(string.format('+%d\t%d\n', time, sample))
    fout:write(string.format('%f\t%f\n', time, sample[1]--[[/32000]]))  --FIXME
    signal[#signal + 1] = {time, sample[1]}
    time = time + timestep_s
    
   --print("timestep_ms = ", timestep_ms)
   -- print("time = ", time)
   --  print("sample = ", sample[1])
  end



--[==[

  local line_num = 1
  for line in fin:lines() do
    local finded = false
    
    if( finded == false ) then
      local cap = {}
      cap[1], cap[2], cap[3], cap[4], cap[5], cap[6], cap[7], cap[8], cap[9], cap[10] = line:match(net_req)
      if ( cap[1] ~= nil ) then
        flog:write(string.format('line_num = %06d\n', line_num))
        flog:write('---'..line..'\n') 
        nets[#nets + 1] = cap[1]
        finded = true
      end
    end

    line_num = line_num + 1
  end

  table.sort(nets, compare)

  for i = 1, #nets do
    fout:write(nets[i]..'\n')
  end
  
  
]==]  
  
  return signal
end





------------ main ------------

if arg[0]:find('wave_2_csv.lua') ~= nil then
  local args = process_args()

  local fout = io.open(args.fout_name,"w");
  if ( fout == nil ) then
    fin:close()
    os.exit(-1)
  end


  local flog = io.open(args.flog_name,"w");
  if ( flog == nil ) then
    fin:close()
    fout:close()
    os.exit(-1)
  end

 -- fout:write(string.format('V1000 VGENp VGENn dc=0 PWL(\n'))
  
  
  print("args.size = "..args.size)
  
  local signal = exec(args.fin_name, fout, flog, tonumber(args.size))

  ArbExpress_2_9_2013_csv_fout(args.fout_name, signal, flog)
 -- fout:write(string.format('+)\n\n'))


  fout:close()
  flog:close()

  os.exit(0)
end


return {
  version = version,
  info = info,
  exec = exec
}


