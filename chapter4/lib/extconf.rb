require 'mkmf'

if have_header('quadmath.h')
  have_func('rb_opts_exception_p', 'ruby.h')
  have_func('cerfq', 'quadmath.h')
  have_func('cerfcq', 'quadmath.h')
  have_func('clgamma', 'quadmath.h')
  have_func('ctgamma', 'quadmath.h')
  $libs << " -lquadmath"
  create_makefile('quadmath')
else
  $stderr.puts "quadmath not found"
end
