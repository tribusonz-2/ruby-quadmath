require 'mkmf'

if have_header('quadmath.h')
  have_func('rb_opts_exception_p', 'ruby.h')
  have_func('cerfq', 'quadmath.h')
  have_func('cerfcq', 'quadmath.h')
  have_func('clgammaq', 'quadmath.h')
  have_func('ctgammaq', 'quadmath.h')
  have_func('cl2norm2q', 'quadmath.h')

  $libs << " -lquadmath"
  create_makefile('quadmath')
else
  $stderr.puts "quadmath not found"
end
