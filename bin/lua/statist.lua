--cp1251


local statist = {}

local luafft = require ("luafft")


local function version()
  return 'v 1.0a'
end


local function info()
  local info = 
[[
  Модуль для статистического анализа *.csv - файлов.
]]
  return info
end


local function process_args()
  -- get args set by user in command line
  local t, i = {}, 1
  while i < #arg do
    local a=arg[i]
    if a == "--fin" then
      t.fin_name = arg[i + 1]
      i = i + 2
    elseif a == "--fout" then
      t.fout_name = arg[i + 1]
      i = i + 2
    elseif a == "--flog" then
      t.flog_name = arg[i + 1]
      i = i + 2
    elseif a == "--fft_size" then
      t.fft_size = arg[i + 1]
      i = i + 2    
    else
      print("Bad flag: "..a)
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


local function RMS(signal)
  local akk = 0
  local RMS = 0
  
  if(type(signal) ~= 'table') then
    return
  end
  
  if( (type(signal[1].time) ~= 'number') and (type(signal[1].value) ~= 'number') ) then
    return
  end  
  
  for i = 1, #signal do
    akk = akk + ((signal[i].value) ^ 2)
  end

  RMS = math.sqrt(akk/#signal)
  return RMS
end


local function THD(signal, file_name, size)
  local gnuplot_pattern =
[[
reset
set encoding cp1251
set term pdfcairo font "Times-New-Roman,10"
set terminal qt 1 noraise enhanced #отключить терминал
set grid
#set key box top right spacing 1.5  #место для легенды

name = "<NAME>"
file_name = "<FILE_NAME>"
fft_size = "<FFT_SIZE>"

v_dB(ref, in) = 20 * log10(in/ref)

set title "График спектра сигнала из файла ".name." (первые ".fft_size." отсчётов)"
set xlabel "Частота, Гц"
set ylabel "Амплитуда, дБ"

stats file_name.'.csv'  using 2 name "A"

plot  file_name.'.csv'   using 1:(v_dB(A_max, $2)) with lines  title 'Amplitude'


set label  "RMS = <RMS> V, \nTHD = <THD> %" at graph  0.1, graph  0.1


set terminal pdfcairo enhanced color notransparent
set output file_name.'.pdf'

replot
unset output
unset terminal
]]


  if(type(signal) ~= 'table') then
    return
  end
  
  if( (type(signal[1].time) ~= 'number') and (type(signal[1].value) ~= 'number') ) then
    return
  end   


  if ( type(size) ~= 'number') then
    size = 2^10
  end
  
  local rms = RMS(signal)
  ------------------------------------------------------------------------------
  -- Hamming function for fft.  w(n) =  0.54 - 0.46 * cos((2 * PI * n)/(M - 1))
  --                                      0 =< n =< (M - 1)
  ------------------------------------------------------------------------------
  local Hamming = {}
  for n = 1, size do
    Hamming[n] = 0.54 - 0.46 * math.cos((2 * math.pi * n)/(size - 1))
  end

  local vec_H = {}
  for i = 1, size do
    vec_H[i] = signal[i].value * Hamming[i]
  end

  local spec = luafft.fft(vec_H, false)
  local sample_rate = (1/signal[2].time - signal[1].time)/size

  local freq = 0
  local spec_abs = {}
  local spectrum_file_name = file_name..'_spectrum'
  local fout = io.open(spectrum_file_name..'.csv', "w");
  for i = 1, (#spec/2) do -- INFO Спектр амплитуд симметричный для реального сигнала
    freq = freq + sample_rate
    spec_abs[i] = spec[i]:abs()
    fout:write(string.format("%f\t%f\n", freq, spec_abs[i]))
  end
  fout:close()


  local U1_width = 2 * 5  -- FIXME 

  for i = 1, (U1_width/2) do --Убрали U0
    spec_abs[i] = 0  
  end

  local max = 0;
  local max_index = 1;
  for i = 1, #spec_abs do 
    if(max < spec_abs[i]) then
      max = spec_abs[i];
      max_index = i;
    end
  end

  local U1 = {}

  local i_min = max_index - (U1_width/2)
  local i_max = max_index + (U1_width/2)
  
  if(i_min <= 0) then
    print('WARNING: i_min <= 0')
    i_min = 1
  end
  
  if(i_max > #spec_abs) then
    print('WARNING: i_max > #spec_abs')
    i_max = #spec_abs
  end  
  
  for i = i_min, i_max do
    U1[#U1 + 1] = spec_abs[i];
    spec_abs[i] = 0
  end


  local Un = 0
  for i = 1, #spec_abs do 
    Un = Un + (spec_abs[i] ^ 2)
  end

  --print (Un)

  local U_1 = 0
  for i = 1, #U1 do 
    U_1 = U_1 + (U1[i] ^ 2)
  end

  --print (U_1)

  local THD = math.sqrt(Un/U_1) * 100
  


  file_plt = io.open(file_name..'_spectrum.plt', "w")
  local plt = gnuplot_pattern:gsub('<FILE_NAME>', spectrum_file_name)
  plt = plt:gsub('<NAME>', spectrum_file_name:gsub('_', '\\\\_')..'.csv')
  plt = plt:gsub('<FFT_SIZE>', string.format("%d", size))
  
  plt = plt:gsub('<RMS>', string.format("%.04f", rms))
  plt = plt:gsub('<THD>', string.format("%.02f", THD))
  file_plt:write(plt)
  file_plt:close()
  os.execute('gnuplot "'..file_name..'_spectrum.plt'..'" ')
  
  
  
  return THD
end





local function exec(fin, fout, flog, file_name, fft_size)
  local signal = {}

  for l in fin:lines() do 
    local fields = trim(l)
    fields = fields:gsub('%s+', '\t')
    fields = split(fields, '\t')

    if( (fields[1] ~= nil) and (fields[2] ~= nil) ) then
      fields[1] = fields[1]:gsub(",", ".")
      fields[1] = tonumber(fields[1])
      fields[2] = fields[2]:gsub(",", ".")
      fields[2] = tonumber(fields[2])
      if( (type(fields[1]) == 'number') and (type(fields[2]) == 'number') ) then
        signal[#signal + 1] = {time = fields[1], value = fields[2]}
      end
    else
      print(l)
    end
  end


  local thd = THD(signal, file_name, fft_size)
  print ('THD = '..thd)
end




------------ main ------------

if arg[0]:find('statist.lua') ~= nil then
  local args = process_args()

  local flog = io.open(args.flog_name,"w");
  if ( flog == nil ) then
    os.exit(-1)
  end

  local fin = io.open(args.fin_name,"r");
  if ( fin == nil ) then
    flog:close()
    os.exit(-1)
  end

  local fout = io.open(args.fout_name,"w");
  if ( fout == nil ) then
    fin:close()
    flog:close()
    os.exit(-1)
  end


  local file_name = args.fout_name:gsub('.[%_%a%d]+$', '')
  print('\n\n============ '..file_name..' ============')
  exec(fin, fout, flog, file_name, tonumber(args.fft_size))

  fin:close()
  fout:close()
  flog:close()

  os.exit(0)
end


return {
  version = version,
  info = info,
  exec = exec
}



