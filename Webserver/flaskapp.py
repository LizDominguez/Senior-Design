from flask import Flask, render_template, abort, g, redirect, url_for, request
from datetime import datetime, timedelta
import sqlite3
import flask_login

DATABASE = '/data/dogs.db'

login_manager = flask_login.LoginManager()

app = Flask(__name__, static_url_path='/static')
app.secret_key = 'lizdominguezleon'
login_manager.init_app(app)
users = {'liz': {'pw': 'dominguez'}, 'miguel': {'pw': 'fernandez'}}

class User(flask_login.UserMixin):
    pass


@login_manager.user_loader
def user_loader(email):
    if email not in users:
        return
    user = User()
    user.id = email
    return user


@login_manager.request_loader
def request_loader(request):
    email = request.form.get('email')
    if email not in users:
        return
    user = User()
    user.id = email
    user.is_authenticated = request.form['pw'] == users[email]['pw']
    return user


@app.route('/login.html', methods=['GET', 'POST'])
def login():
    if request.method == 'GET':
        return render_template('login.html')

    email, pw = request.form['email'], request.form['pw']
    if email not in users:
        return 'invalid username or password <br /> <a href="/">Go back</a>'
    elif users.get(email)['pw'] == pw:
        user = User()
        user.id = email
        flask_login.login_user(user)
        return redirect(url_for('admin'))
    return 'unable to log in <br /> <a href="/">Go back</a>'


@app.route('/logout')
def logout():
    flask_login.logout_user()
    return redirect(url_for('root_handler'))


@app.route('/admin.html', methods=['GET', 'POST'])
@flask_login.login_required
def admin():
    if request.method == 'GET':
        cur = get_db_connection().cursor()
        matrix = [[]]
        for row in cur.execute('SELECT rfid, adopted, image FROM dogs'):
            if len(matrix[-1]) == 3:
                matrix.append([row])
            else:
                matrix[-1].append(row)
        name = flask_login.current_user.id
        return render_template('admin.html', name = name, matrix = matrix)

    original_rfid = request.form['original_rfid']
    new_rfid = request.form['new_rfid']
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('UPDATE dogs SET rfid=? WHERE rfid=?', (new_rfid, original_rfid))
    cur.close()
    connection.commit()
    return redirect(url_for("admin"))


def get_db_connection():
    database = getattr(g, '_database', None)
    if database is None:
        database = g._database = sqlite3.connect(DATABASE, detect_types=sqlite3.PARSE_DECLTYPES)
    return database


@app.teardown_appcontext
def close_database_connection(exception):
    database = getattr(g, '_database', None)
    if database is not None:
        database.close()


@app.route('/')
def root_handler():
    return redirect(url_for('about_page'))


@app.route('/about.html')
def about_page():
    return render_template('about.html')


@app.route('/adopt.html')
def adopt_page():
    cur = get_db_connection().cursor()
    matrix = [[]]
    for row in cur.execute('SELECT rfid, adopted, image FROM dogs'):
        if len(matrix[-1]) == 3:
            matrix.append([row])
        else:
            matrix[-1].append(row)
    cur.close()
    return render_template('adopt.html', matrix=matrix)


@app.route('/add/<rfid>/<action>')
def add_entry(rfid, action):
    adopted = None
    if action == 'a':
        adopted = 1
    elif action == 's':
        adopted = 0
    else:
        abort(400, 'invalid action')
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('UPDATE dogs SET adopted=? WHERE rfid=?', (adopted, rfid))
    cur.close()
    connection.commit()
    return 'OK\r\n'


if __name__ == '__main__':
    app.run('0.0.0.0',80)