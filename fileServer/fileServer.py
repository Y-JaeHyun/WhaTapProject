from flask import Flask, render_template, request
from werkzeug import secure_filename
app = Flask(__name__)


@app.route("/")
def hello():
    return "This is Main"

@app.route('/upload')
def render_file():
    print ("upload.html")
    return render_template('/backup/wogus3314/github/WhaTapProject/fileServer/upload.html')

@app.route('/realtimeUpload', methods = ['POST'])
def realtime_upload():
    if request.method == 'POST':
        with open('backup.csv', "a") as file:
            file.write(request.form['data'])
            
        return ''

@app.route('/uploader', methods = ['GET', 'POST'])
def upload_file():
    if request.method == 'POST':
        f = request.files['file']
        f.save(secure_filename(f.filename))
        return 'file uploaded successfully'

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug = True)
