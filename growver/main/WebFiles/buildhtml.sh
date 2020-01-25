echo 'Building Main HTML'
xxd -i main/Growver_2020.html | sed 's/\([0-9a-f]\)$/\0, 0x00/' > main/growver_html.h
sed -i '1s/^/const /' main/growver_html.h

echo 'Building Other HTML'
xxd -i main/Growver_simple.html | sed 's/\([0-9a-f]\)$/\0, 0x00/' > main/growver_simple_html.h
sed -i '1s/^/const /' main/growver_simple_html.h
