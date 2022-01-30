--cp1251


local sinus_C_array = {}


local function version()
  return 'v 1.0a'
end


local function info()
  local info = 
[[
  Модуль для для создания массива - временной диаграммы простого сигнала.
  
  set file_name=sinus_3400_8000
  set param="sample_rate = 8000, fun = 'sinus', freq = 3400, amp = 2"
  
  rem set file_name=sinus_1800_and_2200_48000
  rem set param="sample_rate = 48000, fun = 'sinus_f1_f2', freq1 = 1800, freq2 = 2200, amp1 = 1, amp2 = 1"

]]
  return info
end


local function process_args()
  -- get args set by user in command line
  local t, i = {}, 1
  while i < #arg do
    local a=arg[i]
    if a == "--fout" then
      t.fout_name = arg[i + 1]
      i = i + 2
    elseif a == "--flog" then
      t.flog_name = arg[i + 1]
      i = i + 2
    elseif a == "--size" then
      t.size = arg[i + 1]
      i = i + 2
    elseif a == "--param" then
      t.param = arg[i + 1]
      i = i + 2      
    else
      print("Bad flag: "..a)
      os.exit(-1)
    end
  end
  return t
end




local preamble_c = 
[[
#include "stdint.h"

const double Signal[2][%d] = 
{
]] 


local preamble_lua = 
[[
Signal = {
]]





local function simple_fout(fout_name, signal, flog)
  local fout_name = fout_name..'.txt'
  local fout = io.open(fout_name,"w");
  if ( fout == nil ) then
    flog:write(string.format('ERROR:  "%s"\n', fout_name))
  end
  
  for i = 1, #signal do
    fout:write(string.format("%e\n", signal[i][2]))
  end

  fout:close()
end


local function simple_csv_fout(fout_name, signal, flog)
  local fout_name = fout_name..'.csv'
  local fout = io.open(fout_name,"w");
  if ( fout == nil ) then
    flog:write(string.format('ERROR:  "%s"\n', fout_name))
  end
   
  local clock = 1/(signal[2][1] - signal[1][1])
  for i = 1, #signal do
    fout:write(string.format("%e\t%e\n", signal[i][1], signal[i][2]))
  end

  fout:close()
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




--[==[
  local fout_c = io.open(fout_name..'.c',"w");
  if ( fout_c == nil ) then
    fout:close()
    flog:close()
    os.exit(-1)
  end

  local fout_lua = io.open(fout_name..'.lua',"w");
  if ( fout_c == nil ) then
    fout_c:close()
    fout:close()
    flog:close()
    os.exit(-1)
  end

]==]

--  fout_c:write(string.format(preamble_c, size))
 -- fout_lua:write(string.format(preamble_lua))

   -- fout:write(string.format("%f\n", signal[#signal][2]))
  --  fout_c:write(string.format("%f,\n", w))
  --  fout_lua:write(string.format("%f,\n", w))

 -- fout_c:write(string.format("};\n"))
--  fout_lua:write(string.format("}\n"))


--[==[
  file_dem = io.open("example.dem", "w")
  file_dem:write(string.format("plot '%s' with lines\n", args.fout_name))
  file_dem:write("pause -1 \"Hit return to continue\"\n")
  file_dem:close()
  os.execute('gnuplot "example.dem" ')
]==]



--[==[
  local size = tonumber(args.size)
  
  if type(size) ~= 'number' then 
  --  fout_lua:close()
  --  fout_c:close()
    fout:close()
    flog:close()
    os.exit(-1)
  end
  
 -- dofile(fsettings_name)
  
]==]  

local function sinus(t, param)
  local omega = 2 * math.pi * param.freq
  local val = param.amp * math.sin(omega * t)
  return val
end


local function sinus_f1_f2(t, param)
  local omega1 = 2 * math.pi * param.freq1
  local omega2 = 2 * math.pi * param.freq2
  
  local val1 = param.amp1 * math.sin(omega1 * t)
  local val2 = param.amp2 * math.sin(omega2 * t)
    
  local val = val1 + val2
  return val
end


local function func(t, param)
  local val = 0
  if(param.fun == 'sinus_f1_f2') then
    val = sinus_f1_f2(t, param)
  elseif(param.fun == 'sinus') then
    val = sinus(t, param)
  elseif(param.fun == 'custom') then
  
  else
  
  end
  return val
end




local function exec(flog, size, param)
  local signal = {}
  local Gz = 1
  local mS = 10^-3
  local sample_rate = param.sample_rate --48000 * Gz
  local record_time = size/sample_rate   --2^16/sample_rate   --200 * mS
  local t = 0
  local dt = 1/sample_rate
  print('dt = ', dt)
  

  while t < record_time do
    signal[#signal + 1] = {t, func(t, param)}
    t = t + dt
  end

  return signal
end


------------ main ------------

if arg[0]:find('sinus_C_array.lua') ~= nil then
  local args = process_args()

  local flog = io.open(args.flog_name,"w");
  if ( flog == nil ) then
    os.exit(-1)
  end

  local fout_name = args.fout_name:gsub('.[%_%a%d]+$', '')


  print("fout_name = ", fout_name)
  print('--------------args.param = '..args.param)
  
  local res, param = pcall(load("return {"..args.param.."}"))
  
  
  print('-----------'..type(param)..'--------')
  
  
  local signal = exec(flog, args.size, param)
  simple_fout(fout_name, signal, flog)
  ArbExpress_2_9_2013_csv_fout(fout_name, signal, flog)
  simple_csv_fout(fout_name, signal, flog)

  flog:close()
  os.exit(0)
end


return {
  version = version,
  info = info,
  exec = exec
}



