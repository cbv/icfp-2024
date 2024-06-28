import { writeFileSync } from 'fs';
import express from 'express';
import * as path from 'path';

const app = express();

app.post('/export', (req, res) => {
  let data = '';
  req.setEncoding('utf8');
  req.on('data', chunk => {
    data += chunk;
  });
  req.on('end', () => {
    const body = JSON.parse(data);
    writeFileSync(path.join(__dirname, '../data.json'), JSON.stringify(body, null, 2) + '\n', 'utf8');
    console.log('Wrote data to data.json');
    res.end('ok');
  });
});
app.use('/', express.static(path.join(__dirname, '../public')));

const PORT = process.env.PORT == undefined ? 8000 : parseInt(process.env.PORT);

app.listen(PORT, '127.0.0.1', function() {
  console.log(`listening on port ${PORT}`);
});
