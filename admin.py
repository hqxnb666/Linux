from flask_admin import Admin,AdminIndexView
from manage import app
from flask_admin.contrib.sqla import ModelView
from flask import current_app,redirect,url_for,request
from models import db,User,Role,Weather
from flask_security import current_user

class MyModelView(ModelView):
    def is_accessible(self):
        if current_user.is_anonymous:
            return False
        for resu in User.query.get(current_user.get_id()).roles:
            if resu.name == 'admin':
                return True
        return False

    def inaccessible_callback(self, name, **kwargs):
        # redirect to login page if user doesn't have access
        return redirect(url_for('index'))

class MyUser(MyModelView):
    column_labels = dict(
        username='账号',
        email='邮箱',
        password='密码',
    )
    column_searchable_list = ['username']


class MyWeather(MyModelView):
    column_searchable_list = ['city']



admin = Admin(app=app, name='功能页',template_mode='bootstrap3', base_template='admin/mybase.html',index_view=AdminIndexView(
        name='导航栏',
        template='admin/welcome.html',
        url='/admin'
    ))
admin.add_view(MyUser(User, db.session,name='用户管理'))
admin.add_view(MyModelView(Role, db.session,name='权限管理'))
admin.add_view(MyWeather(Weather, db.session,name='数据管理'))

if __name__ == '__main__':
    app.run(debug=True)