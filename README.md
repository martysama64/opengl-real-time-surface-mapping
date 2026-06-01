# Визуализация рельефных поверхностей

Этот репозиторий содержит три программы на C++ с использованием OpenGL и SFML, демонстрирующие следующие методы рельефного текстурирования:

1. **Bump Mapping** 
2. **Normal Mapping** 
3. **Displacement Mapping**

В качестве объекта используется сфера

## Управление

<table>
  <tr>
    <th align="center">Клавиша</th>
    <th align="center">Bump Mapping</th>
    <th align="center">Normal Mapping</th>
    <th align="center">Displacement Mapping</th>
  </tr>
  <tr>
    <td align="center"><strong>W/A/S/D</strong></td>
    <td colspan="3" align="center">Перемещение камеры</td>
  </tr>
  <tr>
    <td align="center"><strong>Мышь</strong></td>
    <td colspan="3" align="center">Поворот камеры</td>
  </tr>
  <tr>
    <td align="center"><strong>ESC</strong></td>
    <td colspan="3" align="center">Включение/выключение захвата мыши</td>
  </tr>
  <tr>
    <td align="center"><strong>C</strong></td>
    <td colspan="3" align="center">Выбор цвета сферы</td>
  </tr>
  <tr>
    <td align="center"><strong>L</strong></td>
    <td align="center">Загрузка bump map</td>
    <td align="center">Загрузка normal map</td>
    <td align="center">Загрузка height map</td>
  </tr>
  <tr>
    <td align="center"><strong>B</strong></td>
    <td align="center">Включение/выключение bump mapping</td>
    <td align="center">Включение/выключение normal mapping</td>
    <td align="center">Включение/выключение displacement mapping</td>
  </tr>
  <tr>
    <td align="center"><strong>K</strong></td>
    <td align="center">—</td>
    <td align="center">—</td>
    <td align="center">Загрузка цветовой текстуры</td>
  </tr>
  <tr>
    <td align="center"><strong>T</strong></td>
    <td align="center">—</td>
    <td align="center">—</td>
    <td align="center">Включение/выключение цветовой текстуры</td>
  </tr>
  <tr>
    <td align="center"><strong>Up / Down</strong></td>
    <td align="center">—</td>
    <td align="center">—</td>
    <td align="center">Увеличение/уменьшение глубины displacement mapping</td>
  </tr>
  <tr>
    <td align="center"><strong>+ / -</strong></td>
    <td align="center">—</td>
    <td align="center">—</td>
    <td align="center">Малое увеличение/уменьшение глубины displacement mapping</td>
  </tr>
  <tr>
    <td align="center"><strong>PageUp / PageDown</strong></td>
    <td align="center">—</td>
    <td align="center">—</td>
    <td align="center">Увеличение/уменьшение уровня тесселяции</td>
  </tr>
</table>

## Требования
- OpenGL 4.x
- SFML 3.x
- GLEW
- GLM
