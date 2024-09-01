require 'mkmf'

if have_header('quadmath.h')
  have_func('rb_opts_exception_p', 'ruby.h')
  $libs << " -lquadmath"
  create_makefile('quadmath')
else
  stderr.puts "quadmath not found"
end
